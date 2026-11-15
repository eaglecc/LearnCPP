#include <iostream>

class Shape {
public:
    explicit Shape(const char* name) : name_(name) {
        std::cout << "Shape constructor called for " << name << std::endl;
    }
    ~Shape() {
        std::cout << "Shape destructor called for " << name_ << std::endl;
    }
    const char* name() const {
        std::cout << "Shape::name() called" << std::endl;
        return name_;
    }
private:
    const char* name_;
};

class Circle : public Shape {
public:
    Circle(const char* name, double radius) : Shape(name), radius_(radius) {
        std::cout << "Circle constructor called for " << name << " with radius " << radius << std::endl;
    }
    ~Circle() {
        std::cout << "Circle destructor called for " << name() << std::endl;
    }
    double radius() const {
        std::cout << "Circle::radius() called" << std::endl;
        return radius_;
    }
private:
    double radius_;
};

int main() {
    Circle c("MyCircle", 5.0);
    std::cout << "Circle name: " << c.name() << std::endl;
    std::cout << "Circle radius: " << c.radius() << std::endl;

    return 0;
}