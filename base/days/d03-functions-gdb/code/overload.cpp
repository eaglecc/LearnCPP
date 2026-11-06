#include <cstddef>
#include <iostream>

namespace {

void pick(int a) {
    std::cout << "pick(int): " << a << '\n';
}

void pick(double a) {
    std::cout << "pick(double): " << a << '\n';
}

void pick(float a) {
    std::cout << "pick(float): " << a << '\n';
}

void touch(int& x) {
    std::cout << "touch(int&): " << x << '\n';
}

void touch(const int& x) {
    std::cout << "touch(const int&): " << x << '\n';
}

void ptr(int* p) {
    std::cout << "ptr(int*): " << static_cast<const void*>(p) << '\n';
}

void ptr(std::nullptr_t) {
    std::cout << "ptr(nullptr_t)\n";
}

}  // namespace

int main() {
    std::cout << "== pick ==\n";
    pick(1);
    pick('a');
    pick(1.0f);
    pick(1.0);

    std::cout << "== touch ==\n";
    int x = 1;
    const int cx = 2;
    touch(x);
    touch(cx);
    touch(3);

    std::cout << "== ptr ==\n";
    ptr(nullptr);
    int* p = nullptr;
    ptr(p);

    return 0;
}
