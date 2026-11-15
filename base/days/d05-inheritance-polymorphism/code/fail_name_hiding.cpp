#include <iostream>

// 失败实验：Derived::foo 隐藏了基类的两个 foo，d.foo(1) 无法编译。
class Base {
public:
    void foo(int x) { std::cout << "Base::foo(int) " << x << '\n'; }
    void foo(double x) { std::cout << "Base::foo(double) " << x << '\n'; }
};

class Derived : public Base {
public:
    void foo(const char* s) { std::cout << "Derived::foo(const char*) " << s << '\n'; }
};

int main() {
    Derived d;
    d.foo("hello");  // OK：Derived::foo
    d.foo(42);       // 错误：Base::foo 被隐藏，42 不能转成 const char*
}
