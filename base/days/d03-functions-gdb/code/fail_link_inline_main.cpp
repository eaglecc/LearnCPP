#include "fail_link_inline.h"

#include <iostream>

int fail_inline_a();
int fail_inline_b();

int main() {
    std::cout << fail_inline_a() + fail_inline_b() << '\n';
    return 0;
}
