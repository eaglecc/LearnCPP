#include <iostream>

// 只为观察内部链接符号；[[maybe_unused]] 避免 -Wunused-variable。
[[maybe_unused]] static int file_local = 7;

namespace math {
    int add(int a, int b) {
        return a + b;
    }

    namespace detail {
        int twice(int x) {
            return 2 * x;
        }
    }
}

namespace {
    int hidden_counter = 0;
    void bump() {
        ++hidden_counter;
        std::cout << "Hidden counter: " << hidden_counter << std::endl;
    }
}

int main() {
    int sum = math::add(3, 4);
    int doubleSum = math::detail::twice(sum);
    std::cout << "Sum: " << sum << ", Double Sum: " << doubleSum << std::endl;

    {
        // using 声明：只把 math::add 这一个名字引入当前块作用域。
        using math::add;
        std::cout << "using declaration: " << add(1, 2) << std::endl;
    }

    {
        // using 指令：把 math 里的名字都引入当前块作用域。
        using namespace math;
        std::cout << "using directive: " << add(2, 3) << std::endl;
    }

    bump();
    bump();
    return 0;
}