#include "link_stats.h"

#include <iostream>

int main() {
    const int data[] = {4, 8, 15, 16, 23, 42};
    constexpr int n = 6;

    std::cout << "sum=" << sum(data, n) << '\n';
    std::cout << "average=" << average(data, n) << '\n';

    return 0;
}
