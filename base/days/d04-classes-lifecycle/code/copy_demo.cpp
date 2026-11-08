// 演示：拷贝构造与拷贝赋值分别在什么时候被调用。
#include <iostream>

class Tracker {
public:
    Tracker();
    Tracker(const Tracker& other);
    Tracker& operator=(const Tracker& other);
    ~Tracker();
    int id() const;

private:
    int id_;
    static int next_id_;
};

int Tracker::next_id_ = 0;

Tracker::Tracker() : id_(next_id_++) {
    std::cout << "默认构造   id=" << id_ << '\n';
}

Tracker::Tracker(const Tracker& other) : id_(next_id_++) {
    std::cout << "拷贝构造   id=" << id_ << " <- id=" << other.id_ << '\n';
}

Tracker& Tracker::operator=(const Tracker& other) {
    // id_ 只用来表明是哪个对象，不参与赋值；真实类里这里复制数据 / 资源。
    std::cout << "拷贝赋值   id=" << id_ << " <- id=" << other.id_ << '\n';
    return *this;
}

Tracker::~Tracker() {
    std::cout << "析构       id=" << id_ << '\n';
}

int Tracker::id() const { return id_; }

void take_by_value(Tracker t) {
    std::cout << "  take_by_value 函数体  t.id=" << t.id() << '\n';
}

Tracker make_tracker() {
    return Tracker{};  // prvalue
}

Tracker make_tracker_named() {
    Tracker t;   // 具名局部变量
    return t;    // NRVO（可选优化）
}

int main() {
    std::cout << "== 基本四种写法 ==\n";
    Tracker a;       // 默认构造
    Tracker b = a;   // 拷贝构造（复制初始化）
    Tracker c(a);    // 拷贝构造（直接初始化）
    b = a;           // 拷贝赋值

    std::cout << "\n== 按值传参 ==\n";
    take_by_value(a);

    std::cout << "\n== 按值返回：prvalue ==\n";
    Tracker d = make_tracker();

    std::cout << "\n== 按值返回：具名局部变量 ==\n";
    Tracker e = make_tracker_named();

    std::cout << "\n== 离开 main ==\n";
    return 0;
}
