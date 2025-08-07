#pragma once

#include <cassert>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

namespace backport {

// The feature test macro __cpp_lib_move_only_function is specifically designed to detect the availability of the std::move_only_function
// feature in the standard library, which was introduced in C++23. The value 202110L represents the date when the feature was added to the
// standard (October 2021).
#if defined(__cpp_lib_move_only_function) && __cpp_lib_move_only_function >= 202110L && !defined(MOVE_ONLY_FUNCTION_CUSTOM_IMPL)

// Use std::move_only_function if available
template <typename Signature> using move_only_function = std::move_only_function<Signature>;

#else

// Custom implementation for pre-C++23

// Primary template
template <typename Signature> class move_only_function;

// Helper for invoking with void vs non-void return types
template <typename R, typename F, typename... Args> R invoke_and_return(F &&f, Args &&...args) {
    if constexpr (std::is_void_v<R>) {
        std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    } else {
        return std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    }
}

// Specialization for any function signature R(Args...)
template <typename R, typename... Args> class move_only_function<R(Args...)> {
  private:
    // SOO buffer size - typically 2-3 pointers worth of space
    // Chosen to fit common callables: function pointers, small lambdas, reference_wrapper
    static constexpr std::size_t buffer_size = sizeof(void*) * 3;
    static constexpr std::size_t buffer_align = alignof(std::max_align_t);
    
    // Type-erased interface for callable objects
    struct callable_base {
        virtual R invoke(Args...) = 0;
        virtual void move_to(void* dst) = 0;  // Move construct into destination
        virtual void destroy() noexcept = 0;  // Destroy the callable
        virtual ~callable_base() = default;
    };

    // Implementation for heap-allocated callables
    template <typename Callable> struct heap_callable_impl : callable_base {
        Callable func;

        template <typename F> explicit heap_callable_impl(F &&f) : func(std::forward<F>(f)) {}

        R invoke(Args... args) override { 
            return invoke_and_return<R>(func, std::forward<Args>(args)...); 
        }
        
        void move_to(void* dst) override {
            // For heap callables, we just move the pointer, not the object
            new (dst) heap_callable_impl(std::move(*this));
        }
        
        void destroy() noexcept override {
            // Heap callables are destroyed via delete
        }
    };
    
    // Implementation for inline-stored callables  
    template <typename Callable> struct inline_callable_impl : callable_base {
        Callable func;

        template <typename F> explicit inline_callable_impl(F &&f) : func(std::forward<F>(f)) {}

        R invoke(Args... args) override { 
            return invoke_and_return<R>(func, std::forward<Args>(args)...); 
        }
        
        void move_to(void* dst) override {
            // Move construct into the destination buffer
            new (dst) inline_callable_impl(std::move(*this));
        }
        
        void destroy() noexcept override {
            // Explicitly destroy the callable
            this->~inline_callable_impl();
        }
    };

    // Storage for the callable - either inline buffer or heap pointer
    union storage_t {
        // Aligned buffer for small object optimization
        alignas(buffer_align) std::byte buffer[buffer_size];
        // Pointer for heap-allocated objects (not used directly, just for size)
        void* ptr;
        
        storage_t() noexcept : ptr(nullptr) {}
        ~storage_t() noexcept {}
    } storage;
    
    // Pointer to the vtable - nullptr means empty
    // Points to storage.buffer for inline objects, or heap for large objects  
    callable_base* callable = nullptr;
    
    // Helper to check if object is stored inline
    bool is_inline() const noexcept {
        return callable && 
               reinterpret_cast<const std::byte*>(callable) >= storage.buffer &&
               reinterpret_cast<const std::byte*>(callable) < storage.buffer + buffer_size;
    }
    
    // Helper to determine if a type can use SOO
    template <typename Callable>
    static constexpr bool can_use_soo() {
        return sizeof(inline_callable_impl<Callable>) <= buffer_size &&
               alignof(inline_callable_impl<Callable>) <= buffer_align &&
               std::is_nothrow_move_constructible_v<Callable>;
    }
    
    // Clean up current callable
    void reset() noexcept {
        if (callable) {
            if (is_inline()) {
                callable->destroy();
            } else {
                delete callable;
            }
            callable = nullptr;
        }
    }

  public:
    move_only_function() noexcept = default;

    move_only_function(std::nullptr_t) noexcept : move_only_function() {}

    move_only_function(move_only_function &&other) noexcept : storage{} {
        if (other.callable) {
            if (other.is_inline()) {
                // Move inline object to our buffer
                other.callable->move_to(storage.buffer);
                callable = reinterpret_cast<callable_base*>(storage.buffer);
                // Clear the source
                other.callable->destroy();
                other.callable = nullptr;
            } else {
                // Transfer heap pointer ownership
                callable = other.callable;
                other.callable = nullptr;
            }
        }
    }

    move_only_function(const move_only_function &) = delete;

    template <typename F>
    move_only_function(F &&f)
        requires(!std::is_same_v<std::decay_t<F>, move_only_function> && std::is_invocable_r_v<R, F, Args...>)
    {
        using decayed_type = std::decay_t<F>;
        
        if constexpr (can_use_soo<decayed_type>()) {
            // Use small object optimization - construct directly in buffer
            auto* ptr = new (storage.buffer) inline_callable_impl<decayed_type>(std::forward<F>(f));
            callable = ptr;
        } else {
            // Allocate on heap for large objects
            callable = new heap_callable_impl<decayed_type>(std::forward<F>(f));
        }
    }

    // Destructor
    ~move_only_function() noexcept {
        reset();
    }

    move_only_function &operator=(move_only_function &&other) noexcept {
        if (this != &other) {
            reset();  // Clean up current state
            
            if (other.callable) {
                if (other.is_inline()) {
                    // Move inline object to our buffer
                    other.callable->move_to(storage.buffer);
                    callable = reinterpret_cast<callable_base*>(storage.buffer);
                    // Clear the source
                    other.callable->destroy();
                    other.callable = nullptr;
                } else {
                    // Transfer heap pointer ownership
                    callable = other.callable;
                    other.callable = nullptr;
                }
            }
        }
        return *this;
    }

    move_only_function &operator=(const move_only_function &) = delete;

    // Assignment from nullptr
    move_only_function &operator=(std::nullptr_t) noexcept {
        reset();
        return *this;
    }

    template <typename F>
    move_only_function &operator=(F &&f)
        requires(!std::is_same_v<std::decay_t<F>, move_only_function> && std::is_invocable_r_v<R, F, Args...>)
    {
        // Use move-and-swap idiom for exception safety
        move_only_function tmp(std::forward<F>(f));
        swap(tmp);
        return *this;
    }

    R operator()(Args... args) const {
        assert(callable && "callable is nullptr");
        return callable->invoke(std::forward<Args>(args)...);
    }

    explicit operator bool() const noexcept { return callable != nullptr; }

    // Member swap
    void swap(move_only_function &other) noexcept {
        if (this == &other) return;
        
        // Both inline: swap via temporary buffer
        if (is_inline() && other.is_inline()) {
            alignas(buffer_align) std::byte temp_buffer[buffer_size];
            
            // Move this to temp
            callable->move_to(temp_buffer);
            callable->destroy();
            
            // Move other to this
            other.callable->move_to(storage.buffer);
            callable = reinterpret_cast<callable_base*>(storage.buffer);
            other.callable->destroy();
            
            // Move temp to other
            auto* temp_callable = reinterpret_cast<callable_base*>(temp_buffer);
            temp_callable->move_to(other.storage.buffer);
            other.callable = reinterpret_cast<callable_base*>(other.storage.buffer);
            temp_callable->destroy();
        }
        // One inline, one heap: need careful swap
        else if (is_inline() && !other.is_inline()) {
            // Save heap pointer
            auto* heap_ptr = other.callable;
            
            // Move inline to other's buffer  
            callable->move_to(other.storage.buffer);
            other.callable = reinterpret_cast<callable_base*>(other.storage.buffer);
            callable->destroy();
            
            // Set our pointer to heap
            callable = heap_ptr;
        }
        else if (!is_inline() && other.is_inline()) {
            // Save our heap pointer
            auto* heap_ptr = callable;
            
            // Move other's inline to our buffer
            other.callable->move_to(storage.buffer);
            callable = reinterpret_cast<callable_base*>(storage.buffer);
            other.callable->destroy();
            
            // Set other's pointer to heap
            other.callable = heap_ptr;
        }
        // Both heap: simple pointer swap
        else {
            std::swap(callable, other.callable);
        }
    }
};

// Non-member swap
template <typename Signature> void swap(move_only_function<Signature> &lhs, move_only_function<Signature> &rhs) noexcept { lhs.swap(rhs); }

#endif

} // namespace backport