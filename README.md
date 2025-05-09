
1.  **`<expected>` (`std::expected<T, E>`)**:
    *   **Why:** This is a vocabulary type for error handling, similar to `std::optional` or `std::variant`. It can be implemented using a union and a discriminator flag, or by leveraging `std::variant` (C++17). Several standalone implementations already exist.
    *   **Target:** C++17 (using `std::variant`), C++11/14 (manual implementation with union/discriminator).

2.  **`<utility>` additions like `std::to_underlying`, `std::unreachable`**:
    *   **`std::to_underlying`**: Simple to implement using `static_cast<std::underlying_type_t<Enum>>(e)`. `std::underlying_type_t` is C++11.
    *   **`std::unreachable`**: Can often be mapped to compiler intrinsics (`__builtin_unreachable`, `__assume(0)`) or be a no-op if those aren't available (though with less optimization potential).
    *   **Target:** C++11.

3.  **`<memory>` additions like `std::out_ptr`, `std::inout_ptr`**:
    *   **Why:** These are adaptors for working with C-style APIs that take pointers to pointers. Their implementation is based on RAII and smart pointer mechanics.
    *   **Target:** C++11/14.

4.  **`<type_traits>` additions like `std::is_scoped_enum`**:
    *   **Why:** This is a type trait. Its implementation involves SFINAE or `if constexpr` (C++17) techniques that are well-understood and can often be adapted for older standards.
    *   **Target:** C++11 (with SFINAE) or C++17 (easier with `if constexpr`).

5.  **`<bit>` addition `std::byteswap`**:
    *   **Why:** Can be implemented using compiler intrinsics (like `_byteswap_ushort` etc. on MSVC, `__builtin_bswap16` etc. on GCC/Clang) or with bitwise operations as a fallback.
    *   **Target:** C++11 (intrinsics have been around).

6.  **Monadic operations for `std::optional` (in `<optional>`)**:
    *   **Why:** (`and_then`, `or_else`, `transform`). These are higher-order functions. If `std::optional` (C++17) is available, implementing these is straightforward. If porting to pre-C++17, you'd first need a `std::optional` backport.
    *   **Target:** C++17.

7.  **`<string_view>` and `<string>` additions like `contains`**:
    *   **Why:** The `contains(char)` or `contains(string_view)` methods are simple wrappers around `find(...) != npos`.
    *   **Target:** C++17 (for `std::string_view`) or C++11 (for `std::string`).

**Moderately Portable (More effort, might need C++17/20 features as a base):**

1.  **`<flat_map>`, `<flat_set>` (`std::flat_map`, `std::flat_set`)**:
    *   **Why:** These are containers based on sorted `std::vector`s. The core logic is standard C++, but implementing them correctly and efficiently (especially with all allocator requirements, exception guarantees, etc.) is non-trivial. They don't depend on C++23 *language* features per se.
    *   **Target:** C++11/14 (though C++17 `std::pmr::vector` could be a nice base).

2.  **`<format>` additions like `std::format_to_n`**:
    *   **Why:** Similar to `std::print`, it builds on `std::format`. If `std::format` is available, this is an extension.
    *   **Target:** C++20 (if `std::format` is available), or C++11/14/17 if using a `std::format` backport.

3.  **`<ranges>` additions (e.g., `views::zip`, `views::chunk`, `ranges::to`)**:
    *   **Why:** These build upon the C++20 Ranges library. If you have a C++20 Ranges backport (like range-v3 by Eric Niebler), then adding these new C++23 adaptors and utilities on top is feasible. Porting C++20 Ranges *itself* to pre-C++20 is a very significant undertaking.
    *   **Target:** C++20 (if `<ranges>` is available), or C++11/14/17 if using a comprehensive ranges backport.

4.  **`<functional>` `std::move_only_function`**:
    *   **Why:** A version of `std::function` that works with move-only callables. The type erasure techniques are complex but don't fundamentally require C++23-specific language features. Getting it right is hard.
    *   **Target:** C++11/14.
