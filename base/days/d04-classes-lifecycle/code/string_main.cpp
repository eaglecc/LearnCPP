// 手写 String 的测试：深拷贝、自赋值、链式赋值、源对象销毁后拷贝仍有效。
#include <iostream>

#include "my_string.h"

int main() {
    std::cout << "== 基本内容 ==\n";
    String a("hello");
    String b = a;   // 拷贝构造
    String c;       // 默认构造
    std::cout << "a: " << a.c_str() << " (size=" << a.size() << ")\n";
    std::cout << "b: " << b.c_str() << " (size=" << b.size() << ")\n";
    std::cout << "c: '" << c.c_str() << "' (size=" << c.size()
              << ", empty=" << std::boolalpha << c.empty() << ")\n";

    std::cout << "\n== 深拷贝：地址必须不同 ==\n";
    std::cout << "a.data_ = " << static_cast<const void*>(a.c_str()) << '\n';
    std::cout << "b.data_ = " << static_cast<const void*>(b.c_str()) << '\n';

    std::cout << "\n== 拷贝赋值 ==\n";
    c = a;
    std::cout << "c = a  -> " << c.c_str() << " (size=" << c.size() << ")\n";
    c = c;   // 自赋值
    std::cout << "c = c  -> " << c.c_str() << " (size=" << c.size() << ")\n";

    std::cout << "\n== 链式赋值 ==\n";
    String d("x");
    d = b = a;
    std::cout << "d = b = a -> d=" << d.c_str() << ", b=" << b.c_str() << '\n';

    std::cout << "\n== 源对象销毁后，拷贝仍然有效 ==\n";
    String copy;
    {
        String temp("temporary");
        copy = temp;
    }
    std::cout << "copy = " << copy.c_str() << '\n';
}
