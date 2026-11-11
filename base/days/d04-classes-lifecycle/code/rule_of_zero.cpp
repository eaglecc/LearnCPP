// Rule of 0：成员自己管理资源（std::string），Person 不写任何特殊成员。
#include <iostream>
#include <string>

class Person {
public:
    Person(const std::string& name, int age) : name_(name), age_(age) {}
    const std::string& name() const { return name_; }
    int age() const { return age_; }

private:
    std::string name_;
    int age_;
};

int main() {
    // 64 个字符超过 libstdc++ 的 SSO 容量，确保 std::string 真的用堆内存。
    const std::string long_name(64, 'A');
    Person a(long_name, 30);
    Person b = a;   // 编译器生成的拷贝构造：std::string 自己完成深拷贝

    std::cout << "a: size=" << a.name().size() << ", age=" << a.age() << '\n';
    std::cout << "b: size=" << b.name().size() << ", age=" << b.age() << '\n';
    std::cout << "内容相同: " << std::boolalpha << (a.name() == b.name()) << '\n';
    std::cout << "地址不同: " << (a.name().data() != b.name().data()) << '\n';
    std::cout << "a.data=" << static_cast<const void*>(a.name().data())
              << ", b.data=" << static_cast<const void*>(b.name().data()) << '\n';
}
