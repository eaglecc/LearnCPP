#include <iostream>

class Member {
public:
    explicit Member(const char* label) : label_(label) {
        std::cout << "Member ctor: " << label_ << '\n';
    }
    ~Member() {
        std::cout << "Member dtor: " << label_ << '\n';
    }

private:
    const char* label_;
};

class Shape {
public:
    explicit Shape(const char* name) : name_(name) {
        std::cout << "Shape ctor: " << name_ << '\n';
    }
    ~Shape() {
        std::cout << "Shape dtor: " << name_ << '\n';
    }

private:
    const char* name_;
};

class Circle : public Shape {
public:
    Circle(const char* name, double radius) : Shape(name), tag_("circle.tag") {
        std::cout << "Circle ctor body: " << name << ", r=" << radius << '\n';
    }
    ~Circle() {
        std::cout << "Circle dtor body\n";
    }

private:
    Member tag_;
};

int main() {
    // | 构造  | 基类 `Shape` → 派生类成员 `tag_` → 派生类构造函数体 |
    // | 析构  | 派生类析构函数体 → 派生类成员 `tag_` → 基类 `Shape` |
    Circle c("C", 2.0);
}
