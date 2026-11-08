// 反面教材：不写拷贝构造 / 拷贝赋值，编译器生成的版本只逐成员复制裸指针。
// 用 ASan 运行会报 double-free。不要加进 CMake。
#include <cstring>
#include <iostream>

class BadString {
public:
    explicit BadString(const char* s) {
        const std::size_t n = std::strlen(s);
        data_ = new char[n + 1];
        std::memcpy(data_, s, n + 1);
    }

    ~BadString() { delete[] data_; }

    const char* c_str() const { return data_; }

private:
    char* data_;
};

int main() {
    BadString a("hello");
    BadString b = a;   // 编译器生成的拷贝构造：只复制指针

    std::cout << "a: " << a.c_str() << "  addr="
              << static_cast<const void*>(a.c_str()) << std::endl;
    std::cout << "b: " << b.c_str() << "  addr="
              << static_cast<const void*>(b.c_str()) << std::endl;
    std::cout << "两个对象的 data_ 指向同一块内存" << std::endl;
}
