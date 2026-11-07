#include "fail_extern_const.h"

#include <iostream>

int bad_a();
int bad_b();

int main() {
    std::cout << bad_a() + bad_b() << '\n';
    return 0;
}
