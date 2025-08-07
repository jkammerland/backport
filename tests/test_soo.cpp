#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <backport/move_only_function.hpp>
#include <doctest/doctest.h>
#include <cstddef>
#include <memory>
#include <new>

using namespace backport;

// Global counters to track allocations
static std::size_t allocation_count = 0;
static std::size_t deallocation_count = 0;

// Custom new/delete to track heap allocations
void* operator new(std::size_t size) {
    ++allocation_count;
    void* ptr = std::malloc(size);
    if (!ptr) throw std::bad_alloc();
    return ptr;
}

void operator delete(void* ptr) noexcept {
    if (ptr) {
        ++deallocation_count;
        std::free(ptr);
    }
}

void operator delete(void* ptr, std::size_t) noexcept {
    operator delete(ptr);
}

// Reset allocation counters
void reset_counters() {
    allocation_count = 0;
    deallocation_count = 0;
}

TEST_CASE("SOO: Function pointers should not allocate") {
    reset_counters();
    
    auto func = [](int x) { return x * 2; };
    int (*fptr)(int) = func;
    
    {
        move_only_function<int(int)> mof(fptr);
        CHECK(mof(5) == 10);
    }
    
    // Function pointers should use SOO, no heap allocation
    CHECK(allocation_count == 0);
    CHECK(deallocation_count == 0);
}

TEST_CASE("SOO: Small lambdas should not allocate") {
    reset_counters();
    
    {
        // Small lambda with no captures
        move_only_function<int(int)> mof([](int x) { return x + 1; });
        CHECK(mof(5) == 6);
    }
    
    // Should use SOO for small lambdas
    CHECK(allocation_count == 0);
    CHECK(deallocation_count == 0);
}

TEST_CASE("SOO: Small lambdas with small captures should not allocate") {
    reset_counters();
    
    {
        int value = 42;
        // Lambda with small capture (single int)
        move_only_function<int(int)> mof([value](int x) { return x + value; });
        CHECK(mof(8) == 50);
    }
    
    // Should still use SOO
    CHECK(allocation_count == 0);
    CHECK(deallocation_count == 0);
}

TEST_CASE("SOO: Large lambdas should allocate") {
    reset_counters();
    
    {
        // Lambda with large capture (array)
        int large_data[100] = {0};
        large_data[0] = 1;
        
        move_only_function<int()> mof([large_data]() { return large_data[0]; });
        CHECK(mof() == 1);
    }
    
    // Large lambda should trigger heap allocation
    CHECK(allocation_count > 0);
    CHECK(deallocation_count > 0);
    CHECK(allocation_count == deallocation_count);
}

TEST_CASE("SOO: Move construction preserves storage type") {
    reset_counters();
    
    // Test with small lambda
    {
        move_only_function<int(int)> mof1([](int x) { return x * 3; });
        auto allocs_before_move = allocation_count;
        
        move_only_function<int(int)> mof2(std::move(mof1));
        CHECK(mof2(4) == 12);
        
        // Move of inline storage shouldn't allocate
        CHECK(allocation_count == allocs_before_move);
    }
    
    // Cleanup
    CHECK(allocation_count == deallocation_count);
}

TEST_CASE("SOO: Move assignment preserves storage type") {
    reset_counters();
    
    {
        move_only_function<int(int)> mof1([](int x) { return x * 4; });
        move_only_function<int(int)> mof2([](int x) { return x * 5; });
        
        auto allocs_before = allocation_count;
        
        mof2 = std::move(mof1);
        CHECK(mof2(3) == 12);
        
        // Move assignment of inline objects shouldn't allocate
        CHECK(allocation_count == allocs_before);
    }
    
    CHECK(allocation_count == deallocation_count);
}

TEST_CASE("SOO: Swap works correctly with inline storage") {
    reset_counters();
    
    {
        move_only_function<int(int)> mof1([](int x) { return x + 10; });
        move_only_function<int(int)> mof2([](int x) { return x + 20; });
        
        auto allocs_before = allocation_count;
        
        swap(mof1, mof2);
        
        CHECK(mof1(5) == 25);  // Was mof2's function
        CHECK(mof2(5) == 15);  // Was mof1's function
        
        // Swap of inline objects shouldn't allocate
        CHECK(allocation_count == allocs_before);
    }
    
    CHECK(allocation_count == deallocation_count);
}

TEST_CASE("SOO: Mixed inline/heap swap works correctly") {
    // Create one small (inline) and one large (heap) function
    int small_value = 5;
    int large_data[100] = {0};
    large_data[0] = 100;
    
    move_only_function<int()> small_func([small_value]() { return small_value; });
    move_only_function<int()> large_func([large_data]() { return large_data[0]; });
    
    CHECK(small_func() == 5);
    CHECK(large_func() == 100);
    
    // Swap them
    swap(small_func, large_func);
    
    // Values should be swapped
    CHECK(small_func() == 100);
    CHECK(large_func() == 5);
}