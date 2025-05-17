#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include <backport/move_only_function.hpp>
#include <doctest/doctest.h>
#include <functional>
#include <memory>
#include <string>
#include <utility>

using namespace backport;

// Helper functions and types for tests
int add(int a, int b) { return a + b; }

struct Multiplier {
    int operator()(int a, int b) const { return a * b; }
};

class MoveOnlyFunctor {
  private:
    std::unique_ptr<int> value;

  public:
    MoveOnlyFunctor(int v) : value(std::make_unique<int>(v)) {}
    MoveOnlyFunctor(const MoveOnlyFunctor &)            = delete;
    MoveOnlyFunctor &operator=(const MoveOnlyFunctor &) = delete;
    MoveOnlyFunctor(MoveOnlyFunctor &&)                 = default;
    MoveOnlyFunctor &operator=(MoveOnlyFunctor &&)      = default;

    int operator()(int a) const { return a + *value; }
};

// The actual tests
TEST_CASE("Default construction") {
    move_only_function<void()>      custom_func;
    std::move_only_function<void()> std_func;

    CHECK_FALSE(static_cast<bool>(custom_func));
    CHECK_FALSE(static_cast<bool>(std_func));
}

TEST_CASE("Construction from nullptr") {
    move_only_function<int(int)>      custom_func = nullptr;
    std::move_only_function<int(int)> std_func    = nullptr;

    CHECK_FALSE(static_cast<bool>(custom_func));
    CHECK_FALSE(static_cast<bool>(std_func));
}

TEST_CASE("Construction from lambda") {
    auto lambda = [](int a, int b) { return a + b; };

    move_only_function<int(int, int)>      custom_func = lambda;
    std::move_only_function<int(int, int)> std_func    = lambda;

    CHECK(custom_func(2, 3) == 5);
    CHECK(std_func(2, 3) == 5);
}

TEST_CASE("Construction from free function") {
    move_only_function<int(int, int)>      custom_func = add;
    std::move_only_function<int(int, int)> std_func    = add;

    CHECK(custom_func(4, 5) == 9);
    CHECK(std_func(4, 5) == 9);
}

TEST_CASE("Construction from function object") {
    Multiplier mult;

    move_only_function<int(int, int)>      custom_func = mult;
    std::move_only_function<int(int, int)> std_func    = mult;

    CHECK(custom_func(3, 4) == 12);
    CHECK(std_func(3, 4) == 12);
}

TEST_CASE("Construction from std::function") {
    std::function<int(int)> func = [](int x) { return x * 2; };

    move_only_function<int(int)>      custom_func = func;
    std::move_only_function<int(int)> std_func    = func;

    CHECK(custom_func(5) == 10);
    CHECK(std_func(5) == 10);
}

TEST_CASE("Move construction") {
    auto lambda = [](int x) { return x * 3; };

    move_only_function<int(int)>      custom_func1 = lambda;
    std::move_only_function<int(int)> std_func1    = lambda;

    move_only_function<int(int)>      custom_func2 = std::move(custom_func1);
    std::move_only_function<int(int)> std_func2    = std::move(std_func1);

    CHECK(custom_func2(4) == 12);
    CHECK(std_func2(4) == 12);
}

TEST_CASE("Move assignment") {
    auto lambda1 = [](int x) { return x + 5; };
    auto lambda2 = [](int x) { return x * 5; };

    move_only_function<int(int)>      custom_func1 = lambda1;
    move_only_function<int(int)>      custom_func2 = lambda2;
    std::move_only_function<int(int)> std_func1    = lambda1;
    std::move_only_function<int(int)> std_func2    = lambda2;

    custom_func1 = std::move(custom_func2);
    std_func1    = std::move(std_func2);

    CHECK(custom_func1(3) == 15);
    CHECK(std_func1(3) == 15);
}

TEST_CASE("Assignment from nullptr") {
    auto lambda = [](int x) { return x + 1; };

    move_only_function<int(int)>      custom_func = lambda;
    std::move_only_function<int(int)> std_func    = lambda;

    custom_func = nullptr;
    std_func    = nullptr;

    CHECK_FALSE(static_cast<bool>(custom_func));
    CHECK_FALSE(static_cast<bool>(std_func));
}

TEST_CASE("Assignment from callable") {
    move_only_function<int(int)>      custom_func;
    std::move_only_function<int(int)> std_func;

    auto lambda = [](int x) { return x - 2; };
    custom_func = lambda;
    std_func    = lambda;

    CHECK(custom_func(10) == 8);
    CHECK(std_func(10) == 8);
}

TEST_CASE("Boolean conversion") {
    move_only_function<void()>      custom_func1;
    move_only_function<void()>      custom_func2 = []() {};
    std::move_only_function<void()> std_func1;
    std::move_only_function<void()> std_func2 = []() {};

    CHECK_FALSE(static_cast<bool>(custom_func1));
    CHECK(static_cast<bool>(custom_func2));
    CHECK_FALSE(static_cast<bool>(std_func1));
    CHECK(static_cast<bool>(std_func2));
}

TEST_CASE("Swap member function") {
    move_only_function<int(int)>      custom_func1 = [](int x) { return x * 2; };
    move_only_function<int(int)>      custom_func2 = [](int x) { return x + 10; };
    std::move_only_function<int(int)> std_func1    = [](int x) { return x * 2; };
    std::move_only_function<int(int)> std_func2    = [](int x) { return x + 10; };

    custom_func1.swap(custom_func2);
    std_func1.swap(std_func2);

    CHECK(custom_func1(5) == 15);
    CHECK(custom_func2(5) == 10);
    CHECK(std_func1(5) == 15);
    CHECK(std_func2(5) == 10);
}

TEST_CASE("Swap non-member function") {
    move_only_function<int(int)>      custom_func1 = [](int x) { return x * 3; };
    move_only_function<int(int)>      custom_func2 = [](int x) { return x - 5; };
    std::move_only_function<int(int)> std_func1    = [](int x) { return x * 3; };
    std::move_only_function<int(int)> std_func2    = [](int x) { return x - 5; };

    swap(custom_func1, custom_func2);
    swap(std_func1, std_func2);

    CHECK(custom_func1(10) == 5);
    CHECK(custom_func2(10) == 30);
    CHECK(std_func1(10) == 5);
    CHECK(std_func2(10) == 30);
}

TEST_CASE("Move-only callable") {
    MoveOnlyFunctor              custom_functor(5);
    move_only_function<int(int)> custom_func = std::move(custom_functor);

    MoveOnlyFunctor                   std_functor(5);
    std::move_only_function<int(int)> std_func = std::move(std_functor);

    CHECK(custom_func(10) == 15);
    CHECK(std_func(10) == 15);
}

TEST_CASE("Void return type") {
    int custom_count = 0;
    int std_count    = 0;

    move_only_function<void()>      custom_func = [&]() { custom_count++; };
    std::move_only_function<void()> std_func    = [&]() { std_count++; };

    custom_func();
    std_func();

    CHECK(custom_count == 1);
    CHECK(std_count == 1);
}

TEST_CASE("Multiple arguments") {
    move_only_function<std::string(std::string, int, char)> custom_func = [](std::string s, int n, char c) {
        return s + std::to_string(n) + c;
    };

    std::move_only_function<std::string(std::string, int, char)> std_func = [](std::string s, int n, char c) {
        return s + std::to_string(n) + c;
    };

    CHECK(custom_func("test", 42, '!') == "test42!");
    CHECK(std_func("test", 42, '!') == "test42!");
}

TEST_CASE("Reference arguments") {
    move_only_function<void(int &)>      custom_func = [](int &x) { x *= 2; };
    std::move_only_function<void(int &)> std_func    = [](int &x) { x *= 2; };

    int custom_val = 5;
    int std_val    = 5;

    custom_func(custom_val);
    std_func(std_val);

    CHECK(custom_val == 10);
    CHECK(std_val == 10);
}

TEST_CASE("Exception when calling empty function") {
    move_only_function<int()>      custom_func;
    std::move_only_function<int()> std_func;

    // Both asserts - no throw
}

TEST_CASE("Const reference arguments") {
    move_only_function<int(const std::string &)> custom_func = [](const std::string &s) { return static_cast<int>(s.length()); };

    std::move_only_function<int(const std::string &)> std_func = [](const std::string &s) { return static_cast<int>(s.length()); };

    std::string test = "hello";
    CHECK(custom_func(test) == 5);
}

TEST_CASE("Complex return types") {
    move_only_function<std::unique_ptr<int>()> custom_func = []() { return std::make_unique<int>(42); };

    std::move_only_function<std::unique_ptr<int>()> std_func = []() { return std::make_unique<int>(42); };

    auto custom_result = custom_func();
    auto std_result    = std_func();

    CHECK(*custom_result == 42);
    CHECK(*std_result == 42);
}