#include "link_stats.h"

int sum(const int* data, int n) {
    int total = 0;
    for (int i = 0; i < n; ++i) {
        total += data[i];
    }
    return total;
}

double average(const int* data, int n) {
    if (n <= 0) {
        return 0.0;
    }
    return static_cast<double>(sum(data, n)) / static_cast<double>(n);
}
