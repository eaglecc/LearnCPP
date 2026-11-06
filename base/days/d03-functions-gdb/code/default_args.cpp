#include <iostream>

// 默认值只写在声明里；定义不能重复写。
void log(const char* msg, int level = 0, bool flush = false);

void log(const char* msg, int level, bool flush) {
    std::cout << "log: " << msg
              << ", level=" << level
              << ", flush=" << (flush ? "true" : "false")
              << '\n';
}

int main() {
    log("hello");
    log("hello", 1);
    log("hello", 1, true);
    return 0;
}
