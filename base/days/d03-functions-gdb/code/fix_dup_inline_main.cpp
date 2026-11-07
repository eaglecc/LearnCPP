#include "fix_dup_inline.h"

#include <iostream>

int use_twice(int x);

int main() {
    std::cout << "twice(21)=" << twice(21) << '\n';
    std::cout << "use_twice(10)=" << use_twice(10) << '\n';
    return 0;
}
