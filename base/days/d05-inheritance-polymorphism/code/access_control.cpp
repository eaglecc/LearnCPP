#include <iostream>

// 版本 1：name_ 为 protected —— Circle 可以访问，外部不行。
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
    const char* peek() const { return name_; }  // protected：派生类可以访问

private:
    double radius_;
};

// 版本 2：public —— 派生类和外部都能访问。
class OpenShape {
public:
    explicit OpenShape(const char* name) : name_(name) {}
    const char* name_;

private:
    int reserved_ = 0;
};

class OpenCircle : public OpenShape {
public:
    explicit OpenCircle(const char* name) : OpenShape(name) {}
    const char* peek() const { return name_; }
};

int main() {
    Circle c("protected-circle", 1.0);
    std::cout << "Circle::peek() = " << c.peek() << '\n';
    std::cout << "Shape::name() = " << c.name() << '\n';

    OpenCircle o("public-circle");
    std::cout << "OpenCircle::peek() = " << o.peek() << '\n';
    std::cout << "outside access = " << o.name_ << '\n';
}
