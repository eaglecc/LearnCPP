// D02 任务 1：基本类型与初始化
//
// 普通构建（必须零警告）：
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion types.cpp -o types
//
// UB 演示（单独构建，只有定义了 UB_DEMO 才会执行有符号溢出）：
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -DUB_DEMO
//       -fsanitize=undefined -fno-sanitize-recover=all types.cpp -o types_ub

#include <climits>    // INT_MAX
#include <cstddef>    // std::size_t, std::ptrdiff_t
#include <cstdint>    // std::int8_t ... std::uint64_t
#include <iomanip>    // std::setw, std::left
#include <iostream>
#include <limits>     // std::numeric_limits
#include <type_traits>

namespace{

// 打印整数类型的 sizeof 与 min/max。
// int8_t / uint8_t 的 min()/max() 类型是 (signed/unsigned) char，
// 直接交给 std::cout 会按字符输出；这里的 static_cast 只是为了让“数值”
// 以数字形式打印，不是把窄类型硬塞进 int 来掩盖转换问题。
template <typename T>
void print_limits(const char* name) {
    std::cout << std::left << std::setw(12) << name
              << "sizeof = " << std::setw(2) << sizeof(T) << "  min = ";
    if constexpr (std::is_signed_v<T>) {
        std::cout << static_cast<long long>(std::numeric_limits<T>::min())
                  << ", max = "
                  << static_cast<long long>(std::numeric_limits<T>::max());
    } else {
        std::cout << static_cast<unsigned long long>(std::numeric_limits<T>::min())
                  << ", max = "
                  << static_cast<unsigned long long>(std::numeric_limits<T>::max());
    }
    std::cout << '\n';
}

template <typename T>
void print_layout(const char* name) {
    std::cout << std::left << std::setw(16) << name
              << "sizeof = " << std::setw(2) << sizeof(T)
              << "  alignof = " << alignof(T) << '\n';
}

// 1. 固定宽度类型 + size_t / ptrdiff_t
void fixed_width_types() {
    std::cout << "== 1. 固定宽度整数类型 ==\n";
    print_limits<std::int8_t>("int8_t");
    print_limits<std::int16_t>("int16_t");
    print_limits<std::int32_t>("int32_t");
    print_limits<std::int64_t>("int64_t");
    print_limits<std::uint8_t>("uint8_t");
    print_limits<std::uint16_t>("uint16_t");
    print_limits<std::uint32_t>("uint32_t");
    print_limits<std::uint64_t>("uint64_t");
    print_limits<std::size_t>("size_t");
    print_limits<std::ptrdiff_t>("ptrdiff_t");
    std::cout << '\n';
}

// 2. 大小与对齐表
void layout_table() {
    std::cout << "== 2. 大小与对齐 ==\n";
    print_layout<bool>("bool");
    print_layout<char>("char");
    print_layout<short>("short");
    print_layout<int>("int");
    print_layout<long>("long");
    print_layout<long long>("long long");
    print_layout<float>("float");
    print_layout<double>("double");
    print_layout<long double>("long double");
    print_layout<void*>("void* (pointer)");
    print_layout<std::size_t>("size_t");
    std::cout << '\n';
}

// 3. 四种初始化
void init_forms() {
    std::cout << "== 3. 四种初始化 ==\n";

    // 默认初始化：函数内的 int 不做任何初始化，值是不确定的（indeterminate）。
    // 读它属于未定义行为，所以这里只声明、不读取。想得到 0 请用 int a{};
    [[maybe_unused]] int a;

    int b = 1;    // 拷贝初始化
    int c(1);     // 直接初始化
    int d{1};     // 列表初始化（C++11，禁止窄化）

    std::cout << "b = " << b << ", c = " << c << ", d = " << d << '\n';
    std::cout << "（int a; 未读取：函数内默认初始化的 int 值不确定，读取即 UB）\n\n";
}

#define UB_DEMO 1
// 5. 符号与溢出
void overflow_demo() {
    std::cout << "== 5. 符号与溢出 ==\n";

    // 无符号溢出：定义良好，按 2^N 取模回绕。
    unsigned u = std::numeric_limits<unsigned>::max();
    std::cout << "unsigned max        = " << u << '\n';
    u += 1;
    std::cout << "unsigned max + 1    = " << u << "  （定义良好，回绕）\n";

#ifdef UB_DEMO
    // 有符号溢出：UB。结果不可预测，下面这行打印出来的值没有意义。
    int x = INT_MAX;
    x += 1;  // UB
    std::cout << "INT_MAX + 1         = " << x << "  （UB，别依赖这个值！）\n";
#else
    std::cout << "（UB_DEMO 未定义，跳过有符号溢出演示）\n";
#endif
    std::cout << '\n';
}

// 6. 字符与字符串字面量
void string_literal_demo() {
    std::cout << "== 6. 字符串字面量 ==\n";

    // "abc" 的类型是 const char[4]（结尾还有一个 '\0'）。
    // 字符串字面量是左值，所以 decltype 得到的是引用类型。
    static_assert(sizeof("abc") == 4);
    static_assert(std::is_same_v<std::remove_reference_t<decltype("abc")>,
                                 const char[4]>);

    const char* s = "abc";  // OK：数组退化为指向首元素的 const char*
    // char* bad = "abc";   // 错误：丢弃 const；见 fail_string_literal.cpp

    char copy[] = "abc";    // OK：用字面量初始化一份可写的副本（char[4]）
    copy[0] = 'A';          // 字面量本身不可改，副本可以改

    std::cout << "s = " << s << ", copy = " << copy << '\n';
    std::cout << "（\"abc\" 是 const char[4]，可读不可写；char* 会丢掉 const）\n";
}

}  // namespace

int main() {
    fixed_width_types();
    layout_table();
    init_forms();
    overflow_demo();
    string_literal_demo();
    return 0;
}
