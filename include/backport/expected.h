#pragma once

// The feature test macro __cpp_lib_expected is specifically designed to detect the availability of the std::expected
// feature in the standard library, which was introduced in C++23. The value 202202L represents the date when the
// feature was added to the standard (February 2022).
#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L && !defined(EXPECTED_CUSTOM_IMPL)

// Use std::expected if available
#include <expected>
template <typename T, typename E> using expected = std::expected<T, E>;
template <typename E> using unexpected = std::unexpected<E>;

#else

// Fall back to tl::expected
#include <tl/expected.hpp>
template <typename T, typename E> using expected = tl::expected<T, E>;
template <typename E> using unexpected = tl::unexpected<E>;

#endif
