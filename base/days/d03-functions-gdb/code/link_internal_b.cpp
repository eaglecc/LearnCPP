#include "link_internal.h"

// 和 a.cpp 同名、同类型，但因为内部链接，不会和 a.cpp 的 counter 冲突。
static int counter = 0;

namespace {
// 和 a.cpp 同名，但同样只在 b.cpp 这个 TU 内可见。
int helper() {
    return 42;
}
}  // namespace

int b_value() {
    return counter + helper();
}

void b_increment() {
    ++counter;
}
