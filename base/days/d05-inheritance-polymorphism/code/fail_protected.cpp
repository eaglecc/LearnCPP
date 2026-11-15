#include <iostream>

// 失败实验：name_ 为 private，Circle 里的 peek() 访问它会编译失败。
// 同时演示：protected 成员在类外访问也会失败。
class Shape {
public:
    explicit Shape(const char* name) : name_(name) {}
    const char* name() const { return name_; }

protected:
    const char* name_;
};

class Circle : public Shape {
public:
    Circle(const char* name, double radius) : Shape(name), radius_(radius) {}
    const char* peek() const { return name_; }  // protected：OK
    double radius() const { return radius_; }

private:
    double radius_;
};

class ClosedShape {
public:
    explicit ClosedShape(const char* name) : name_(name) {}

private:
    const char* name_;
};

class ClosedCircle : public ClosedShape {
public:
    explicit ClosedCircle(const char* name) : ClosedShape(name) {}
    const char* peek() const { return name_; }  // 错误：private 成员，派生类不可见
};

int main() {
    Circle c("circle", 1.0);
    std::cout << c.name_ << '\n';  // 错误：protected 成员，类外不可访问
    ClosedCircle cc("closed");
    std::cout << cc.peek() << '\n';
}
