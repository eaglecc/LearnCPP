// 用 -Woverloaded-virtual 观察：派生类同名函数隐藏基类虚函数。
class Base {
public:
    virtual void foo(int x);
    virtual ~Base() = default;
};

class Derived : public Base {
public:
    void foo(double x);  // 隐藏 Base::foo(int)，且没有 override
};

void Base::foo(int) {}
void Derived::foo(double) {}
