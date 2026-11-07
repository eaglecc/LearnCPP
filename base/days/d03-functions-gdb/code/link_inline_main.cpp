#include "link_inline.h"

#include <iostream>

int inline_a();
int inline_b();

int main() {
    std::cout << "inline_a=" << inline_a()
              << ", inline_b=" << inline_b()
              << ", main twice(3)=" << twice(3) << '\n';
    return 0;
}
