// D02 任务 3：引用与指针
//
// 普通构建（必须零警告）：
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror pointers.cpp -o pointers
//
// 悬空指针 ASan 演示（单独构建；-Wno-return-local-addr 只是为了让不安全的演示能编译）：
//   g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -DASAN_DEMO -Wno-return-local-addr -fsanitize=address -g pointers.cpp -o pointers_asan
//   ./pointers_asan

#include <cstddef>
#include <iostream>
#include <vector>

namespace {

void print_array(const int* arr, std::size_t n, const char* label) {
    std::cout << label << ": ";
    for (std::size_t i = 0; i < n; ++i) {
        std::cout << arr[i] << ' ';
    }
    std::cout << '\n';
}

// 1. 交换两个 int：指针版与引用版
void swap_ptr(int* a, int* b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void swap_ref(int& a, int& b) {
    int tmp = a;
    a = b;
    b = tmp;
}

// 2. 求数组最大值：指针遍历 + 下标遍历
int max_ptr(const int* arr, std::size_t n) {
    if (n == 0) {
        return 0;  // 空数组没有最大值；真实接口应约定或返回 optional
    }
    int max = *arr;
    for (const int* p = arr + 1; p != arr + n; ++p) {
        if (*p > max) {
            max = *p;
        }
    }
    return max;
}

int max_of(const int* arr, std::size_t n) {
    if (n == 0) {
        return 0;
    }
    int max = arr[0];
    for (std::size_t i = 1; i < n; ++i) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}

// 3. 原地反转：指针版 + 引用到数组版
void reverse_ptr(int* arr, std::size_t n) {
    if (n < 2) {
        return;
    }
    int* left = arr;
    int* right = arr + n - 1;
    while (left < right) {
        int tmp = *left;
        *left = *right;
        *right = tmp;
        ++left;
        --right;
    }
}

// 固定长度 5 只是为了演示“引用到数组”；泛化需要模板，今天不碰。
void reverse_ref(int (&arr)[5]) {
    for (std::size_t i = 0; i < 2; ++i) {
        int tmp = arr[i];
        arr[i] = arr[4 - i];
        arr[4 - i] = tmp;
    }
}

// 4. 统计某个值出现的次数
std::size_t count_ptr(const int* arr, std::size_t n, int value) {
    std::size_t count = 0;
    for (const int* p = arr; p != arr + n; ++p) {
        if (*p == value) {
            ++count;
        }
    }
    return count;
}

// 引用版：const 引用避免拷贝，range-for 逐个比较。
std::size_t count_ref(const std::vector<int>& v, int value) {
    std::size_t count = 0;
    for (int x : v) {
        if (x == value) {
            ++count;
        }
    }
    return count;
}

// 5. const 正确性：const 引用 vs 按值传参
// 故意分开命名：若写成同名重载 process(const vector<int>&) 与 process(vector<int>)，
// 对左值调用会二义（g++ 13.3.0 已确认），无法各调用一次做对比。
void process_ref(const std::vector<int>& v) {
    std::cout << "process_ref:  data = " << static_cast<const void*>(v.data())
              << ", v[0] = " << v[0] << '\n';
}

void process_val(std::vector<int> v) {
    v[0] = -1;  // 改的是副本，不影响调用方
    std::cout << "process_val:  data = " << static_cast<const void*>(v.data())
              << ", v[0] = " << v[0] << '\n';
}

// 6. 数组退化
void show_size(const int* arr) {
    std::cout << "函数内 sizeof(arr)   = " << sizeof(arr) << "（arr 已退化为指针）\n";
}

void show_size_ref(const int (&arr)[5]) {
    std::cout << "引用数组 sizeof(arr) = " << sizeof(arr) << "（引用到数组，未退化）\n";
}

// #define ASAN_DEMO
#ifdef ASAN_DEMO
// 7. 悬空指针：返回局部变量地址
int* dangling_return() {
    int local = 42;
    return &local;  // 函数返回后 local 已销毁；返回它的地址是 UB
}

// 另一个把局部地址“带出去”的版本。
// GCC 可能把上面的 return &local 直接折成 nullptr（UB 允许），
// 这个版本能让 ASan 给出清晰的 stack-use-after-return 报告。
int* g_escaped = nullptr;

void escape_local() {
    int local = 42;
    g_escaped = &local;
}
#endif

}  // namespace

int main() {
    std::cout << "== 1. swap ==\n";
    int x = 1;
    int y = 2;
    swap_ptr(&x, &y);
    std::cout << "after swap_ptr: x = " << x << ", y = " << y << '\n';
    swap_ref(x, y);
    std::cout << "after swap_ref: x = " << x << ", y = " << y << '\n';

    std::cout << "== 2. max ==\n";
    int nums[] = {3, 1, 4, 1, 5, 9, 2, 6};
    const std::size_t n = sizeof(nums) / sizeof(nums[0]);
    std::cout << "max_ptr = " << max_ptr(nums, n) << '\n';
    std::cout << "max_of  = " << max_of(nums, n) << '\n';

    std::cout << "== 3. reverse ==\n";
    constexpr std::size_t kSize = 5;
    int a[kSize] = {1, 2, 3, 4, 5};
    print_array(a, kSize, "before");
    reverse_ptr(a, kSize);
    print_array(a, kSize, "after reverse_ptr");
    int b[kSize] = {1, 2, 3, 4, 5};
    reverse_ref(b);
    print_array(b, kSize, "after reverse_ref");

    std::cout << "== 4. count ==\n";
    std::vector<int> vnums = {3, 1, 4, 1, 5, 9, 2, 6};
    std::cout << "count_ptr of 1 = " << count_ptr(nums, n, 1) << '\n';
    std::cout << "count_ref of 1 = " << count_ref(vnums, 1) << '\n';

    std::cout << "== 5. const 正确性 ==\n";
    std::vector<int> data = {10, 20, 30};
    std::cout << "main before:  data = " << static_cast<const void*>(data.data())
              << ", data[0] = " << data[0] << '\n';
    process_ref(data);
    process_val(data);
    std::cout << "main after:   data = " << static_cast<const void*>(data.data())
              << ", data[0] = " << data[0] << '\n';

    std::cout << "== 6. 数组退化 ==\n";
    int arr[kSize] = {1, 2, 3, 4, 5};
    std::cout << "main 里 sizeof(arr)  = " << sizeof(arr) << '\n';
    show_size(arr);
    show_size_ref(arr);

#ifdef ASAN_DEMO
    std::cout << "== 7. 悬空指针（ASan） ==\n";
    int* bad = dangling_return();
    std::cout << "dangling_return() = " << static_cast<const void*>(bad)
              << "（不要解引用：UB）\n"
              << std::flush;  // ASan abort 前把缓冲区刷出来

    escape_local();
    std::cout << "*g_escaped = " << *g_escaped << '\n';  // ASan 在这里报错
#endif

    return 0;
}
