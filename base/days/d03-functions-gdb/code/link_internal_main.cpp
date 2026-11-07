#include "link_internal.h"

#include <iostream>

int main() {
    std::cout << "a_value=" << a_value()
              << ", b_value=" << b_value() << '\n';

    a_increment();
    std::cout << "after a_increment: a_value=" << a_value()
              << ", b_value=" << b_value() << '\n';

    b_increment();
    b_increment();
    std::cout << "after b_increment twice: a_value=" << a_value()
              << ", b_value=" << b_value() << '\n';

    return 0;
}
