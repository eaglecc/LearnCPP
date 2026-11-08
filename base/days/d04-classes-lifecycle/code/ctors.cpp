#include <iostream>

class Point {
public:
    Point();                                    // 只声明，定义在类外
    Point(int x, int y) : x_(x), y_(y) {};      // 定义在类内（行尾分号是多余的空声明）
    void print() const;

private:
    int x_;
    int y_;
};

Point::Point() : x_(0), y_(0) {}                // 类外定义，不带行尾分号

void Point::print() const {
    std::cout << "Point(" << x_ << ", " << y_ << ")\n";
}

class Config {
public:
    Config(int id, int& value) : id_(id), value_(value) {}

    int id() const { return id_; }
    int& value() const { return value_; }

private:
    const int id_;
    int& value_;
};

class Trace {
public:
    explicit Trace(const char* name) : name_(name) {
        std::cout << "Trace ctor: " << name_ << '\n';
    }

    ~Trace() {
        std::cout << "Trace dtor: " << name_ << '\n';
    }

private:
    const char* name_;
};

class Holder {
public:
    Holder() : first_("holder.first"), second_("holder.second") {
        std::cout << "Holder ctor\n";
    }

    ~Holder() {
        std::cout << "Holder dtor\n";
    }

private:
    Trace first_;
    Trace second_;
};

class NonCopyable {
public:
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

int main() {
    std::cout << "== Point ==\n";
    Point origin;
    Point p(3, 4);
    origin.print();
    p.print();

    std::cout << "\n== Config ==\n";
    int v = 10;
    Config cfg(42, v);
    std::cout << "Config id=" << cfg.id() << ", value=" << cfg.value() << '\n';
    cfg.value() = 99;
    std::cout << "v after cfg.value() = 99: " << v << '\n';

    std::cout << "\n== Trace: local objects and nested scope ==\n";
    {
        Trace a("a");
        Trace b("b");
        {
            Trace c("c");
        }
        Trace d("d");
    }

    std::cout << "\n== Holder: member objects ==\n";
    {
        Holder h;
    }

    std::cout << "\n== NonCopyable ==\n";
    NonCopyable nc;
    static_cast<void>(nc);
    std::cout << "NonCopyable constructed\n";

    return 0;
}
