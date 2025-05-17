#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "backport/expected.hpp" // SUT: backport::expected
#include "doctest/doctest.h"

#include <expected>

TEST_CASE("Construction with value") {
    int                                  value = 42;
    backport::expected<int, std::string> e1    = value;
    std::expected<int, std::string>      e2    = value;

    CHECK(e1.has_value());
    CHECK(e2.has_value());
    CHECK(*e1 == *e2);
}

TEST_CASE("Construction with error") {
    std::string                          error_message = "error";
    backport::expected<int, std::string> e1            = backport::unexpected<std::string>(error_message);
    std::expected<int, std::string>      e2            = std::unexpected<std::string>(error_message);

    CHECK_FALSE(e1.has_value());
    CHECK_FALSE(e2.has_value());
    CHECK(e1.error() == e2.error());
}

TEST_CASE("Boolean conversion") {
    int         value         = 42;
    std::string error_message = "error";

    backport::expected<int, std::string> e1 = value;
    std::expected<int, std::string>      e2 = value;

    backport::expected<int, std::string> e3 = backport::unexpected<std::string>(error_message);
    std::expected<int, std::string>      e4 = std::unexpected<std::string>(error_message);

    CHECK(static_cast<bool>(e1) == static_cast<bool>(e2));
    CHECK(static_cast<bool>(e3) == static_cast<bool>(e4));
}

TEST_CASE("Equality comparison") {
    int         value         = 42;
    std::string error_message = "error";

    backport::expected<int, std::string> e1 = value;
    std::expected<int, std::string>      e2 = value;

    backport::expected<int, std::string> e3 = backport::unexpected<std::string>(error_message);
    std::expected<int, std::string>      e4 = std::unexpected<std::string>(error_message);

    CHECK(e1.value() == e2.value());
    CHECK(e3.error() == e4.error());
}

TEST_CASE("Move construction") {
    std::string error_message = "error";

    backport::expected<int, std::string> e1 = backport::unexpected<std::string>(error_message);
    backport::expected<int, std::string> e2 = std::move(e1);

    std::expected<int, std::string> e3 = std::unexpected<std::string>(error_message);
    std::expected<int, std::string> e4 = std::move(e3);

    CHECK_FALSE(e2.has_value());
    CHECK_FALSE(e4.has_value());
    CHECK(e2.error() == e4.error());
}

TEST_CASE("Copy construction") {
    int value = 42;

    backport::expected<int, std::string> e1 = value;
    backport::expected<int, std::string> e2 = e1;

    std::expected<int, std::string> e3 = value;
    std::expected<int, std::string> e4 = e3;

    CHECK(e2.has_value());
    CHECK(e4.has_value());
    CHECK(*e2 == *e4);
}

TEST_CASE("Assignment") {
    int value = 42;

    backport::expected<int, std::string> e1;
    e1 = value;

    std::expected<int, std::string> e2;
    e2 = value;

    CHECK(e1.has_value());
    CHECK(e2.has_value());
    CHECK(*e1 == *e2);
}

TEST_CASE("Error type") {
    std::string error_message = "error";
    int         error_value   = 42;

    backport::expected<int, std::string> e1 = backport::unexpected<std::string>(error_message);
    std::expected<int, std::string>      e2 = std::unexpected<std::string>(error_message);

    backport::expected<int, int> e3 = backport::unexpected<int>(error_value);
    std::expected<int, int>      e4 = std::unexpected<int>(error_value);

    CHECK(e1.error() == e2.error());
    CHECK(e3.error() == e4.error());
}

TEST_CASE("Exception when accessing invalid state" * doctest::may_fail()) {
    backport::expected<int, std::string> e     = backport::unexpected<std::string>("error");
    std::expected<int, std::string>      std_e = std::unexpected<std::string>("error");

    CHECK_THROWS_AS(e.value(), backport::bad_expected_access<std::string>);
    CHECK_THROWS_AS(std_e.value(), std::bad_expected_access<std::string>);

    backport::expected<void, std::string> e2     = backport::unexpected<std::string>("error");
    std::expected<void, std::string>      std_e2 = std::unexpected<std::string>("error");

    CHECK_FALSE("TODO: incompatible API : CHECK_THROWS_AS(e2.value(), backport::bad_expected_access<void>); ");
    // CHECK_THROWS_AS(e2.value(), backport::bad_expected_access<void>);

    CHECK_THROWS_AS(std_e2.value(), std::bad_expected_access<void>);
}

TEST_CASE("Monadic operations") {
    // and_then
    backport::expected<int, std::string> e1 = 21;
    auto                                 e2 = e1.and_then([](int val) { return backport::expected<double, std::string>(val * 2.0); });
    CHECK(e2.has_value());
    CHECK(*e2 == 42.0);

    // or_else
    backport::expected<int, std::string> e3 = backport::unexpected<std::string>("error");
    auto                                 e4 = e3.or_else([](const std::string &err) { return backport::expected<int, std::string>(42); });
    CHECK(e4.has_value());
    CHECK(*e4 == 42);

    // transform
    backport::expected<int, std::string> e5 = 21;
    auto                                 e6 = e5.transform([](int val) { return val * 2; });
    CHECK(e6.has_value());
    CHECK(*e6 == 42);

    // transform_error
    backport::expected<int, std::string> e7 = backport::unexpected<std::string>("error");
    auto                                 e8 = e7.transform_error([](const std::string &) { return 42; });
    CHECK(!e8.has_value());
    CHECK(e8.error() == 42);
}

TEST_CASE("Void value type") {
    backport::expected<void, std::string> e1;
    std::expected<void, std::string>      std_e1;

    CHECK(e1.has_value());
    CHECK(std_e1.has_value());

    backport::expected<void, std::string> e2     = backport::unexpected<std::string>("error");
    std::expected<void, std::string>      std_e2 = std::unexpected<std::string>("error");

    CHECK(!e2.has_value());
    CHECK(!std_e2.has_value());
    CHECK(e2.error() == "error");
    CHECK(std_e2.error() == "error");
}

TEST_CASE("Value_or method") {
    backport::expected<int, std::string> e1     = 42;
    std::expected<int, std::string>      std_e1 = 42;

    CHECK(e1.value_or(0) == std_e1.value_or(0));

    backport::expected<int, std::string> e2     = backport::unexpected<std::string>("error");
    std::expected<int, std::string>      std_e2 = std::unexpected<std::string>("error");

    CHECK(e2.value_or(42) == std_e2.value_or(42));
}

TEST_CASE("Non-trivial types") {
    struct NonTrivial {
        std::string data;
        NonTrivial(std::string s) : data(std::move(s)) {}
        bool operator==(const NonTrivial &other) const { return data == other.data; }
    };

    backport::expected<NonTrivial, int> e1{NonTrivial{"test"}};
    std::expected<NonTrivial, int>      std_e1{NonTrivial{"test"}};

    CHECK(e1.has_value());
    CHECK(std_e1.has_value());
    CHECK(e1->data == "test");
    CHECK(std_e1->data == "test");
}

TEST_CASE("In-place construction") {
    backport::expected<std::string, int> e1{backport::in_place, "test"};
    std::expected<std::string, int>      std_e1{std::in_place, "test"};

    CHECK(e1.has_value());
    CHECK(std_e1.has_value());
    CHECK(*e1 == "test");
    CHECK(*std_e1 == "test");

    backport::expected<int, std::string> e2{backport::unexpect, "error"};
    std::expected<int, std::string>      std_e2{std::unexpect, "error"};

    CHECK(!e2.has_value());
    CHECK(!std_e2.has_value());
    CHECK(e2.error() == "error");
    CHECK(std_e2.error() == "error");
}

TEST_CASE("Self-assignment and swap") {
    backport::expected<int, std::string> e1 = 42;
    e1                                      = e1;
    CHECK(e1.has_value());
    CHECK(*e1 == 42);

    backport::expected<int, std::string> e2 = backport::unexpected<std::string>("error");
    backport::expected<int, std::string> e3 = 42;

    swap(e2, e3);
    CHECK(e2.has_value());
    CHECK(*e2 == 42);
    CHECK(!e3.has_value());
    CHECK(e3.error() == "error");
}