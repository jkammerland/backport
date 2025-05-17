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