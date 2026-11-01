// 故意编译失败：C++11 起字符串字面量是 const char[N]，
// 不能隐式转换成 char*（会丢掉 const）。
//
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -c fail_string_literal.cpp
#include <stdio.h>
#include <cstring>

int main() {
    // char* s = "abc";  // 错误：const char[4] -> char* 丢弃 const
    const char *t = "abcdef";  // OK：const char[7] -> const char*
    char copy[] = "abcdef"; // OK：创建一个可写的副本（char[7]）
    printf("%ld\n", sizeof(t)); // 指针的大小，通常是 4 或 8
    printf("%ld\n", sizeof(*t)); // 指针指向的类型的大小，这里是char，通常是 1
    printf("%c\n", t[0]); // 打印第一个字符
    printf("%ld\n", strlen(t)); // 字符串的长度
    printf("%ld\n", sizeof(copy)); // 可编辑的数组
    printf("%ld\n", sizeof(copy) / sizeof(copy[0]) - 1); // 数组元素个数
    return t[0];
}
