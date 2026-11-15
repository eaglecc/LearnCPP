#include <iostream>

// using Base::foo 把基类重载重新引入派生类作用域。
class Base {
public:
    void foo(int x) { std::cout << "Base::foo(int) " << x << '\n'; }
    void foo(double x) { std::cout << "Base::foo(double) " << x << '\n'; }
};

class Derived : public Base {
public:
    using Base::foo;
    void foo(const char* s) { std::cout << "Derived::foo(const char*) " << s << '\n'; }
};

int main() {
    Derived d;
    d.foo("hello");
    d.foo(42);    // 现在能看到 Base::foo(int)
    d.foo(3.14);  // 现在能看到 Base::foo(double)
}
