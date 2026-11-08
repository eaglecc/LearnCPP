// 验证：声明 / = default / 空函数体 / 初始化列表，分别“初始化”了什么。
//
// 编译时加 -ftrivial-auto-var-init=pattern，让没有被初始化的自动变量
// 被填成 0xFE 模式（打印为 -16843010），这样能直接看出“没有初始化”，
// 而不是读到随机值。去掉这个 flag，读到的就是不确定值（UB）。
#include <iostream>
#include <type_traits>

class Declared {
public:
    Declared();  // 只声明，定义在类外
    int a;
    int b;
};

Declared::Declared() {}  // 空函数体：a、b 默认初始化 -> 不确定值

class Defaulted {
public:
    Defaulted() = default;  // 在首次声明处 default：a、b 仍默认初始化 -> 不确定值
    int a;
    int b;
};

class OutOfClassDefault {
public:
    OutOfClassDefault();  // 先声明
    int a;
    int b;
};

OutOfClassDefault::OutOfClassDefault() = default;  // 类外 default：算 user-provided

class WithInitList {
public:
    WithInitList() : a(0), b(0) {}
    int a;
    int b;
};

class WithMemberInit {
public:
    WithMemberInit() = default;
    int a = 0;  // 默认成员初始化器
    int b = 0;
};

// 在首次声明处 = default -> 保持 trivial；类外再 = default -> user-provided。
static_assert(std::is_trivially_default_constructible_v<Defaulted>);
static_assert(!std::is_trivially_default_constructible_v<Declared>);
static_assert(!std::is_trivially_default_constructible_v<OutOfClassDefault>);

int main() {
    std::cout << "== 默认初始化：T x; ==\n";
    Declared d;
    Defaulted e;
    WithInitList f;
    WithMemberInit g;
    std::cout << "Declared      : a=" << d.a << ", b=" << d.b << '\n';
    std::cout << "Defaulted     : a=" << e.a << ", b=" << e.b << '\n';
    std::cout << "WithInitList  : a=" << f.a << ", b=" << f.b << '\n';
    std::cout << "WithMemberInit: a=" << g.a << ", b=" << g.b << '\n';

    std::cout << "\n== 值初始化：T x{}; ==\n";
    Declared d2{};
    Defaulted e2{};
    OutOfClassDefault h2{};
    std::cout << "Declared{}         : a=" << d2.a << ", b=" << d2.b << '\n';
    std::cout << "Defaulted{}        : a=" << e2.a << ", b=" << e2.b << '\n';
    std::cout << "OutOfClassDefault{}: a=" << h2.a << ", b=" << h2.b << '\n';
    return 0;
}
