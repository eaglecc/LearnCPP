#include "link_internal.h"

// static 命名空间作用域变量：内部链接，只属于本 TU。
static int counter = 0;

namespace {
// 匿名 namespace：helper 也是内部链接，只属于本 TU。
int helper() {
    return 42;
}
}  // namespace

int a_value() {
    return counter + helper();
}

void a_increment() {
    ++counter;
}
