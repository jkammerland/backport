#include "backport/move_only_function.hpp"

#include <cassert>
int main() {
    auto f = backport::move_only_function<void()>{};
    assert(!f);
    f = [] {};
    f();
    return 0;
}