#include "backport/expected.hpp"

#include <cassert>
struct A {
    int a;
    int b;
};

int main() {
    // std::expected<void, int> e = std::unexpected(42);
    backport::expected<void, int> e{backport::unexpect, 42};
    if (e) {
        assert(false);
    }
    // auto [a, b] = A{}; // Should warn
    return 0;
}