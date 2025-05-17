# Backport c++

[![Linux CI](https://github.com/jkammerland/backport/actions/workflows/ubuntu_ci.yml/badge.svg?branch=master)](https://github.com/jkammerland/backport/actions/workflows/ubuntu_ci.yml)
[![Windows CI](https://github.com/jkammerland/backport/actions/workflows/windows_ci.yml/badge.svg?branch=master)](https://github.com/jkammerland/backport/actions/workflows/windows_ci.yml)
[![macOS CI](https://github.com/jkammerland/backport/actions/workflows/macos_ci.yml/badge.svg?branch=master)](https://github.com/jkammerland/backport/actions/workflows/macos_ci.yml)

This project aims to backport c++23/26 constructs that can be implemented in earlier versions of C++. The goal is that 
when the real implementation is available, the library defaults to the real one, providing a seamless drop-in replacement.

Take expected as an example. This library use "tl::expected" when the c++ one is not available. Under the hood it works something like this:

```cpp
namespace backport
{
#if defined(__cpp_lib_expected) && __cpp_lib_expected >= 202202L && !defined(EXPECTED_CUSTOM_IMPL)
    template <typename T, typename E>
    using expected = std::expected<T,E>;
    // ... more stuff ...
#else
    template <typename T, typename E>
    using expected = tl::expected<T,E>;
    // ... more stuff ...
#endif
}
```

Currently, these are the backported constructs:

- [x] `std::move_only_function` (c++23 -> c++20)
- [x] `std::expected` (c++23 -> c++11)

## How to install

```cmake
include(FetchContent)
FetchContent_Declare(
    backport
    https://github.com/jkammerland/backport.git
    GIT_TAG v1.0.1 # or branch/commit
)
FetchContent_MakeAvailable(backport)

# ...
target_link_libraries(my_target PRIVATE backport::backport)
```

## How to Use

Using Backport C++ is straightforward - simply include the header for the feature you need and use it from the `backport` namespace:

```cpp
#include <backport/expected.hpp>
#include <backport/move_only_function.hpp>

// Use backport::expected
backport::expected<int, std::string> compute(bool succeed) {
    if (succeed)
        return 42;
    return backport::unexpected("failed");
}

// Use backport::move_only_function
void use_callback(backport::move_only_function<int(int, int)> callback) {
    int result = callback(2, 3);
    // ...
}
```

### What to Expect with Different Compiler Versions

#### C++11 to C++20

When using a compiler that doesn't natively support C++23 features:

- `backport::expected` will use the TartanLlama implementation internally
- `backport::move_only_function` is available if you're using C++20 or later
- All features will work with the same interface as their standard counterparts

#### C++23 and Beyond

When using a compiler with native support for C++23 features:

- `backport::expected` automatically becomes an alias for `std::expected`
- `backport::move_only_function` automatically becomes an alias for `std::move_only_function`
- No runtime or compile-time overhead compared to using the standard library directly

#### Force Custom Implementation

If you need to use the backported implementations even when standard ones are available (for testing purposes or to ensure consistent behavior):

```cpp
// Define before including headers:
#define EXPECTED_CUSTOM_IMPL
#define MOVE_ONLY_FUNCTION_CUSTOM_IMPL

#include <backport/expected.hpp>
#include <backport/move_only_function.hpp>
```

This will force the use of the custom implementations regardless of compiler support. These are also used for testing parity in the unit tests.
```