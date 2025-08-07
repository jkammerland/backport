#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <doctest/doctest.h>
#include <backport/move_only_function.hpp>
#include <cstring>
#include <cstdlib>
#include <memory>
#include <new>
#include <vector>
#include <stdexcept>
#include <array>

#ifdef _MSC_VER
#include <malloc.h>  // For _aligned_malloc/_aligned_free on MSVC
#endif

using namespace backport;

// Track allocations
static int allocation_count = 0;
static int deallocation_count = 0;
static bool tracking_enabled = false;

void* operator new(std::size_t size) {
    if (tracking_enabled) {
        allocation_count++;
    }
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc{};
    return ptr;
}

void* operator new(std::size_t size, std::align_val_t align) {
    if (tracking_enabled) {
        allocation_count++;
    }
    size_t alignment = static_cast<size_t>(align);
#ifdef _MSC_VER
    // MSVC doesn't have std::aligned_alloc, use _aligned_malloc instead
    void* ptr = _aligned_malloc(size, alignment);
#else
    // POSIX/C11 aligned_alloc requires size to be multiple of alignment
    size_t aligned_size = (size + alignment - 1) & ~(alignment - 1);
    void* ptr = std::aligned_alloc(alignment, aligned_size);
#endif
    if (!ptr) throw std::bad_alloc{};
    return ptr;
}

void operator delete(void* ptr) noexcept {
    if (tracking_enabled) {
        deallocation_count++;
    }
    std::free(ptr);
}

void operator delete(void* ptr, std::size_t) noexcept {
    if (tracking_enabled) {
        deallocation_count++;
    }
    std::free(ptr);
}

void operator delete(void* ptr, std::align_val_t) noexcept {
    if (tracking_enabled) {
        deallocation_count++;
    }
#ifdef _MSC_VER
    // MSVC requires _aligned_free for memory allocated with _aligned_malloc
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

void operator delete(void* ptr, std::size_t, std::align_val_t) noexcept {
    if (tracking_enabled) {
        deallocation_count++;
    }
#ifdef _MSC_VER
    // MSVC requires _aligned_free for memory allocated with _aligned_malloc
    _aligned_free(ptr);
#else
    std::free(ptr);
#endif
}

void reset_counters() {
    allocation_count = 0;
    deallocation_count = 0;
}

void enable_tracking() { tracking_enabled = true; }
void disable_tracking() { tracking_enabled = false; }

// Test with exact buffer size object
struct ExactSizeCallable {
    // Make this exactly 24 bytes (3 pointers) minus vtable overhead
    alignas(8) char data[16];  // Adjusted for vtable pointer
    int result;
    
    ExactSizeCallable(int r = 42) : result(r) {
        std::memset(data, 0, sizeof(data));
    }
    
    int operator()() const { return result; }
};

// Test with larger alignment requirements
struct alignas(32) OverAlignedCallable {
    int value;
    int operator()() const { return value; }
};

// Test throwing move constructor
struct ThrowingMoveCallable {
    int value;
    bool moved = false;
    
    ThrowingMoveCallable(int v) : value(v) {}
    
    ThrowingMoveCallable(ThrowingMoveCallable&& other) noexcept(false) {
        if (other.moved) {
            throw std::runtime_error("Already moved!");
        }
        value = other.value;
        other.moved = true;
    }
    
    int operator()() const { return value; }
};

// Test non-trivial destructor
struct DestructorTracker {
    static int destruction_count;
    int value;
    
    DestructorTracker(int v) : value(v) {}
    ~DestructorTracker() { destruction_count++; }
    
    int operator()() const { return value; }
};
int DestructorTracker::destruction_count = 0;

// Test self-referential callable
struct SelfReferential {
    int* ptr;
    int value;
    
    SelfReferential(int v) : value(v), ptr(&value) {}
    
    // Custom move that updates self-reference
    SelfReferential(SelfReferential&& other) noexcept 
        : value(other.value), ptr(&value) {
        other.ptr = nullptr;
    }
    
    int operator()() const { 
        return ptr ? *ptr : -1; 
    }
};

// Test mutable callable
struct MutableCallable {
    mutable int call_count = 0;
    
    int operator()() const {
        return ++call_count;
    }
};

// Large callable to force heap allocation
struct LargeCallable {
    char data[256];
    int value;
    
    LargeCallable(int v) : value(v) {
        std::memset(data, 0, sizeof(data));
    }
    
    int operator()() const { return value; }
};

TEST_SUITE("Edge Cases") {
    TEST_CASE("Exact buffer size boundary") {
        reset_counters();
        enable_tracking();
        {
            move_only_function<int()> mof(ExactSizeCallable{100});
            CHECK(mof() == 100);
        }
        disable_tracking();
        // Should fit in buffer or allocate - implementation dependent
        // Just verify no leaks
        CHECK(allocation_count == deallocation_count);
    }
    
    TEST_CASE("Over-aligned types require heap allocation") {
        reset_counters();
        enable_tracking();
        {
            move_only_function<int()> mof(OverAlignedCallable{42});
            CHECK(mof() == 42);
        }
        disable_tracking();
        // Over-aligned should force heap allocation
        CHECK(allocation_count > 0);
        CHECK(allocation_count == deallocation_count);
    }
    
    TEST_CASE("Throwing move constructor forces heap allocation") {
        reset_counters();
        enable_tracking();
        {
            move_only_function<int()> mof(ThrowingMoveCallable{42});
            CHECK(mof() == 42);
        }
        disable_tracking();
        // Should use heap due to noexcept(false) move constructor
        CHECK(allocation_count > 0);
        CHECK(allocation_count == deallocation_count);
    }
    
    TEST_CASE("Destructor is called correctly") {
        DestructorTracker::destruction_count = 0;
        {
            move_only_function<int()> mof(DestructorTracker{42});
            CHECK(mof() == 42);
        }
        // Expect 2: one for the temporary, one for the stored object
        CHECK(DestructorTracker::destruction_count == 2);
        
        // Test with move - create object first to avoid temporary
        DestructorTracker::destruction_count = 0;
        {
            DestructorTracker tracker{42};
            move_only_function<int()> mof1(std::move(tracker));
            move_only_function<int()> mof2(std::move(mof1));
            CHECK(mof2() == 42);
        }
        // Expect 3: original tracker, moved-from object in mof1 (destroyed during move), final in mof2
        CHECK(DestructorTracker::destruction_count == 3);
    }
    
    TEST_CASE("Self-referential objects maintain validity") {
        move_only_function<int()> mof1(SelfReferential{42});
        CHECK(mof1() == 42);
        
        // After move, should still work
        move_only_function<int()> mof2(std::move(mof1));
        CHECK(mof2() == 42);
        
        // Test swap
        move_only_function<int()> mof3(SelfReferential{100});
        mof2.swap(mof3);
        CHECK(mof2() == 100);
        CHECK(mof3() == 42);
    }
    
    TEST_CASE("Mutable callable state is preserved") {
        move_only_function<int()> mof(MutableCallable{});
        CHECK(mof() == 1);
        CHECK(mof() == 2);
        CHECK(mof() == 3);
        
        // Move should preserve state
        move_only_function<int()> mof2(std::move(mof));
        CHECK(mof2() == 4);
        CHECK(mof2() == 5);
    }
    
    TEST_CASE("Mixed swap scenarios") {
        reset_counters();
        enable_tracking();
        
        // Small (inline) and large (heap) swap
        move_only_function<int()> small_fn([]{ return 42; });
        move_only_function<int()> large_fn(LargeCallable{100});
        
        small_fn.swap(large_fn);
        CHECK(small_fn() == 100);
        CHECK(large_fn() == 42);
        
        // Swap back
        large_fn.swap(small_fn);
        CHECK(small_fn() == 42);
        CHECK(large_fn() == 100);
        
        disable_tracking();
    }
    
    TEST_CASE("Empty function behavior") {
        move_only_function<int()> empty;
        CHECK_FALSE(empty);
        
        // Moving from empty
        move_only_function<int()> mof(std::move(empty));
        CHECK_FALSE(mof);
        
        // Moving to empty
        move_only_function<int()> non_empty([]{ return 42; });
        empty = std::move(non_empty);
        CHECK(empty);
        CHECK(empty() == 42);
        CHECK_FALSE(non_empty);
    }
    
    TEST_CASE("Self-assignment") {
        move_only_function<int()> mof([]{ return 42; });
        auto* ptr = &mof;
        mof = std::move(*ptr);  // Self move-assignment
        CHECK(mof);  // Should remain valid
        CHECK(mof() == 42);
    }
    
    TEST_CASE("Self-swap") {
        move_only_function<int()> mof([]{ return 42; });
        mof.swap(mof);  // Self-swap
        CHECK(mof() == 42);  // Should remain unchanged
    }
    
    TEST_CASE("Assignment from nullptr") {
        move_only_function<int()> mof([]{ return 42; });
        CHECK(mof);
        
        mof = nullptr;
        CHECK_FALSE(mof);
        
        // Assigning nullptr to empty
        mof = nullptr;
        CHECK_FALSE(mof);
    }
    
    TEST_CASE("Chain of moves") {
        move_only_function<int()> mof1([]{ return 42; });
        move_only_function<int()> mof2(std::move(mof1));
        move_only_function<int()> mof3(std::move(mof2));
        move_only_function<int()> mof4(std::move(mof3));
        
        CHECK_FALSE(mof1);
        CHECK_FALSE(mof2);
        CHECK_FALSE(mof3);
        CHECK(mof4);
        CHECK(mof4() == 42);
    }
    
    TEST_CASE("Capturing lambdas with different sizes") {
        reset_counters();
        enable_tracking();
        
        // Small capture - should use SOO
        int x = 42;
        move_only_function<int()> small([x]{ return x; });
        auto small_allocs = allocation_count;
        
        // Medium capture - might use SOO
        int y = 100;
        double z = 3.14;
        move_only_function<int()> medium([x, y, z]{ return x + y + static_cast<int>(z); });
        auto medium_allocs = allocation_count - small_allocs;
        
        // Large capture - should use heap
        std::array<int, 100> arr{};
        arr[0] = 42;
        move_only_function<int()> large([arr]{ return arr[0]; });
        auto large_allocs = allocation_count - small_allocs - medium_allocs;
        
        CHECK(small() == 42);
        CHECK(medium() == 145);
        CHECK(large() == 42);
        
        // Large should definitely allocate
        CHECK(large_allocs > 0);
        
        disable_tracking();
    }
    
    TEST_CASE("Function pointer storage") {
        int global_value = 42;
        
        auto free_func = []() -> int { return 42; };
        int (*func_ptr)() = free_func;
        
        reset_counters();
        enable_tracking();
        {
            move_only_function<int()> mof(func_ptr);
            CHECK(mof() == 42);
        }
        disable_tracking();
        // Function pointers should use SOO
        CHECK(allocation_count == 0);
    }
    
    TEST_CASE("Reference wrapper") {
        auto callable = []{ return 42; };
        
        reset_counters();
        enable_tracking();
        {
            move_only_function<int()> mof(std::ref(callable));
            CHECK(mof() == 42);
        }
        disable_tracking();
        // Reference wrapper should use SOO
        CHECK(allocation_count == 0);
    }
    
    TEST_CASE("Move-only types") {
        struct MoveOnlyCallable {
            std::unique_ptr<int> ptr;
            
            MoveOnlyCallable() : ptr(std::make_unique<int>(42)) {}
            MoveOnlyCallable(MoveOnlyCallable&&) = default;
            MoveOnlyCallable(const MoveOnlyCallable&) = delete;
            
            int operator()() const { return *ptr; }
        };
        
        move_only_function<int()> mof(MoveOnlyCallable{});
        CHECK(mof() == 42);
        
        // Move should work
        move_only_function<int()> mof2(std::move(mof));
        CHECK(mof2() == 42);
    }
    
    TEST_CASE("Exception safety in assignment") {
        static int throw_count = 0;
        struct ThrowOnConstruct {
            ThrowOnConstruct() {
                if (++throw_count > 1) {
                    throw std::runtime_error("Construction failed");
                }
            }
            int operator()() const { return 42; }
        };
        throw_count = 0;
        
        move_only_function<int()> mof([]{ return 100; });
        
        try {
            mof = ThrowOnConstruct{};  // First succeeds
            CHECK(mof() == 42);
            
            mof = ThrowOnConstruct{};  // Second throws
            FAIL("Should have thrown");
        } catch (const std::runtime_error&) {
            // mof should still be valid with previous value
            CHECK(mof);
            CHECK(mof() == 42);  // Should have the first ThrowOnConstruct
        }
    }
}

// Test different return types and argument types
TEST_SUITE("Type Variations") {
    TEST_CASE("Void return type") {
        int counter = 0;
        move_only_function<void(int&)> mof([](int& c) { c++; });
        mof(counter);
        CHECK(counter == 1);
    }
    
    TEST_CASE("Reference return type") {
        int value = 42;
        move_only_function<int&()> mof([&value]() -> int& { return value; });
        mof() = 100;
        CHECK(value == 100);
    }
    
    TEST_CASE("Multiple arguments") {
        move_only_function<int(int, int, int)> mof([](int a, int b, int c) {
            return a + b + c;
        });
        CHECK(mof(1, 2, 3) == 6);
    }
    
    TEST_CASE("Move-only arguments") {
        move_only_function<int(std::unique_ptr<int>)> mof([](std::unique_ptr<int> ptr) {
            return *ptr;
        });
        CHECK(mof(std::make_unique<int>(42)) == 42);
    }
    
    TEST_CASE("Perfect forwarding") {
        struct Counter {
            mutable int copy_count = 0;
            mutable int move_count = 0;
            
            Counter() = default;
            Counter(const Counter& other) : copy_count(other.copy_count + 1), move_count(other.move_count) {}
            Counter(Counter&& other) noexcept : copy_count(other.copy_count), move_count(other.move_count + 1) {}
        };
        
        move_only_function<void(Counter)> mof([](Counter c) {
            CHECK(c.move_count > 0);  // Should be moved, not copied
            CHECK(c.copy_count == 0);
        });
        
        mof(Counter{});
    }
}