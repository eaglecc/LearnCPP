// 故意编译失败：列表初始化禁止窄化（narrowing）。
// 不要加入 CMake，也不要“修好”它——它存在的意义就是让你看到报错原文。
//
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -c fail_narrowing.cpp
//
// 对照：int x = 3.14; 只是隐式转换（-Wconversion 下警告），
//       int x{3.14}; 是硬错误。

int main() {
    int x{3.14};   // 错误：double -> int 会丢失小数部分
    char c{1000};  // 错误：1000 超出 char 的表示范围
    (void)x;
    (void)c;
    return 0;
}
