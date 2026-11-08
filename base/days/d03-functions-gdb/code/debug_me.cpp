#include <iostream>

namespace {
    int sum(const int *data, int n) {
        int total = 0;
        for (int i = 0; i < n - 1; ++i) {
            total += data[i];
        }
        return total;
    }

    double average(const int *data, int n) {
        if (n <= 0) {
            return 0.0;
        }
        return static_cast<double>(sum(data, n)) / static_cast<double>(n);
    }
}


int main() {
    const int data[] = {1, 2, 3, 4, 5};
    int n = sizeof(data) / sizeof(data[0]);
    std::cout << "Sum: " << sum(data, n) << std::endl;
    std::cout << "Average: " << average(data, n) << std::endl;
    return 0;
}