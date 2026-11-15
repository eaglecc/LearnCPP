# D06 任务：运算符重载 + C++11/14/17

> 预计 7 小时。概念 2h / 动手 4h / 笔记与 Git 1h。
> 环境：Ubuntu 24.04（WSL2 或 README 里的虚拟机），GCC 13+，CMake 3.28+，Ninja，ASan。
> 构建目录统一用 `~/build/base`，源码在 `base/days/d06-operators-modern-cpp/code/`。

D05 让对象通过基类指针表现出多态。今天解决两个问题：

> **怎么让自定义类型像 `int` 一样自然地参与运算和输出？**
> **C++11/14/17 的哪些特性改变了日常写法？**

围绕这两个问题，完成七件事：

1. 运算符重载的基本规则：成员 vs 非成员、对称性、`const`；
2. `operator<<`、比较运算符与 `<=>`；
3. 类型转换运算符与 `explicit`；
4. lambda、`auto` 与 C++11 常用特性；
5. 移动语义入门（D10 再深入）；
6. C++14/17 标准库特性：`make_unique`、structured bindings、`if constexpr`、`optional`、`variant`、`string_view`、`filesystem`、`[[nodiscard]]`、fold expressions；
7. 完整实现 `Vec3`，写成可复用的头文件 + 实现文件。

## 今日目标

- [ ] 能判断一个运算符该写成成员还是非成员，并说清理由
- [ ] 能写出对称的 `+`、`-`、`*`，以及可链式调用的 `+=`
- [ ] 能实现 `operator<<`、`operator==`、`operator<=>`
- [ ] 会用 `explicit operator bool()`，知道它和普通转换函数的区别
- [ ] 掌握 lambda 的捕获方式、`mutable`、初始化捕获、泛型 lambda
- [ ] 能写出移动构造 / 移动赋值，理解 `std::move` 只是类型转换
- [ ] 会用 `make_unique`、structured bindings、`if constexpr`、`optional`、`variant`、`string_view`、`filesystem`
- [ ] 完成 `Vec3` 完整实现，零警告
- [ ] CMake 多 target、Git 提交、笔记完成

## 今日不碰

- 模板元编程、concepts、ranges、STL 算法深入（D7 / D8）
- allocator、智能指针深入、循环引用（D9；今天只基础使用 `make_unique`）
- 异常安全等级、`std::forward` / 完美转发、引用折叠（D10）
- 并发（D11）
- 继承与多态（D5 已完成）
- 重载 `new` / `delete`、重载 `&&` / `||` / `,`（通常不该做）

> 本目录的 `code/` 已经建好；所有练习文件放进去。
> 下面的命令如果没有特别说明，默认在 `base/days/d06-operators-modern-cpp/code/` 下执行；CMake 命令在仓库根执行。
> 失败实验（`fail_*.cpp`）只用来记录错误，不进 CMake。

---

## 任务 1：运算符重载基础与 `Vec3`（约 80 min）

### 1.1 哪些运算符必须是成员

先记住硬规则：

| 运算符 | 能否写成非成员 |
|---|---|
| `=`（赋值） | 必须是成员 |
| `[]` | 必须是成员（C++20） |
| `()` | 必须是成员（C++20） |
| `->` | 必须是成员 |
| 类型转换函数 `operator T()` | 必须是成员 |
| `+`、`-`、`*`、`==`、`<<` 等 | 可以是非成员，通常也建议非成员 |

其余大多数运算符两种写法都可以，选择标准是**对称性**：

- `a + b` 和 `b + a` 应该得到同样结果 → `operator+` 写成非成员（或 `friend`）；
- `a += b` 修改左操作数 → `operator+=` 写成成员，返回 `*this`；
- 流运算符左边是 `std::ostream`，不可能是成员 → 必须是非成员。

### 1.2 `Vec3` 的骨架

在 `code/vec3.h` 里写：

```cpp
#pragma once
#include <cstddef>
#include <iosfwd>

class Vec3 {
public:
    Vec3() = default;
    Vec3(double x, double y, double z);

    double x() const;
    double y() const;
    double z() const;

    double operator[](std::size_t index) const;

    Vec3& operator+=(const Vec3& rhs);
    Vec3& operator-=(const Vec3& rhs);

    Vec3 operator-() const;

    double dot(const Vec3& rhs) const;
    double length() const;

    bool operator==(const Vec3& rhs) const = default;
    auto operator<=>(const Vec3& rhs) const = default;

private:
    double x_ = 0.0;
    double y_ = 0.0;
    double z_ = 0.0;
};

Vec3 operator+(Vec3 lhs, const Vec3& rhs);
Vec3 operator-(Vec3 lhs, const Vec3& rhs);
Vec3 operator*(Vec3 lhs, double scalar);
Vec3 operator*(double scalar, Vec3 rhs);
std::ostream& operator<<(std::ostream& os, const Vec3& v);
```

实现放在 `code/vec3.cpp`。

要求：

- `operator+` 用“按值接收左操作数 + `+=`”的写法，减少重复代码；
- `operator+=` 返回 `Vec3&`，支持 `a += b += c`；
- 所有不修改对象的成员函数加 `const`；
- `operator[]` 越界时 `throw std::out_of_range`（D10 再系统讲异常，这里先用）；
- `x()` / `y()` / `z()` 只读访问，不暴露可写引用。

### 1.3 测试

`code/vec3_main.cpp` 至少覆盖：

```cpp
Vec3 a(1, 2, 3);
Vec3 b(4, 5, 6);

Vec3 c = a + b;
Vec3 d = a - b;
Vec3 e = -a;
Vec3 f = 2.0 * a;
Vec3 g = a * 2.0;

a += b;
a -= b;
std::cout << a << ' ' << c << '\n';
std::cout << (a == b) << ' ' << (a != b) << '\n';
std::cout << a[0] << ' ' << a.dot(b) << ' ' << a.length() << '\n';
```

记录输出，并检查：

- `2.0 * a` 和 `a * 2.0` 都可用 → 两个非成员重载；
- `a += b += c` 能否编译？返回类型对不对？
- `a < b` 是否可用？来自 `<=>` 的改写规则。

### 1.4 不该重载的运算符

在笔记里说明为什么通常不重载：

- `&&`、`||`：重载后失去短路求值；
- `,`：几乎总是破坏可读性；
- `&`（取地址）除非写底层库；
- `new` / `delete` 除非做内存池或调试分配器。

---

## 任务 2：流运算符、比较与 `<=>`（约 70 min）

### 2.1 `operator<<`

```cpp
std::ostream& operator<<(std::ostream& os, const Vec3& v) {
    return os << '(' << v.x() << ", " << v.y() << ", " << v.z() << ')';
}
```

要求：

- 返回 `std::ostream&`，保证 `std::cout << a << b` 可链式；
- 第一个参数是 `std::ostream&`，第二个是 `const Vec3&`；
- 不要 `using namespace std;`；
- 在 `vec3.h` 里用 `#include <iosfwd>` 声明，在 `.cpp` 里包含 `<ostream>`。

### 2.2 `operator>>`（可选）

实现 `std::istream& operator>>(std::istream&, Vec3&)`，支持输入 `1 2 3`。读取失败时设置 `failbit`，不要留下半改的对象。

### 2.3 `operator==` 与 `operator<=>`

C++20 起：

```cpp
bool operator==(const Vec3& rhs) const = default;
auto operator<=>(const Vec3& rhs) const = default;
```

- `= default` 的 `==` 按成员逐个比较；
- `= default` 的 `<=>` 按成员声明顺序做字典序比较，并**同时隐式生成 `==`**；
- 成员里有 `double` 时，`<=>` 的返回类型是 `std::partial_ordering`（因为 NaN 不可比），所以用 `auto` 最省事；
- 只写 `<` 和 `==` 不够：`a <= b` 会直接编译失败（实测 `no match for 'operator<='`）；用 `<=>` 才能一次得到 `<`、`<=`、`>`、`>=` 全套比较。

验证：

```cpp
Vec3 a(1, 2, 3);
Vec3 b(1, 2, 4);
std::cout << (a < b) << ' ' << (a <= b) << ' ' << (a > b) << '\n';
```

记录输出，并说明字典序是怎么比较的：先比 `x_`，相等再比 `y_`，再比 `z_`。

### 2.4 成员 vs 非成员：再确认一次

| 写法 | 推荐形式 | 原因 |
|---|---|---|
| `a + b` | 非成员，`Vec3 operator+(Vec3, const Vec3&)` | 左右对称，支持隐式转换 |
| `a += b` | 成员，返回 `Vec3&` | 修改左操作数 |
| `std::cout << v` | 非成员 | 左操作数是流 |
| `v == w` | 成员或非成员均可；`= default` 时写成成员 | 编译器生成逐个比较 |
| `v[i]` | 成员 | 语言要求 |
| `(v)` 或 `v()` | 成员 | 语言要求 |

---

## 任务 3：类型转换运算符与 `explicit`（约 40 min）

### 3.1 `explicit operator bool`

写一个小类 `code/conversion_demo.cpp`：

```cpp
class Connection {
public:
    explicit Connection(bool open);
    explicit operator bool() const;   // 注意 explicit
private:
    bool open_;
};
```

验证：

```cpp
Connection c(true);

if (c) { /* 可以：语境转换 */ }
bool b = c;              // 错误：explicit 阻止复制初始化
bool b2 = static_cast<bool>(c);   // 可以
```

记录编译错误。

说明：

- `explicit operator bool()` 允许 `if (c)`、`while (c)`、`!c`、`c && ...` 这类**语境转换**；
- 阻止 `bool b = c;`、`int n = c;` 这类隐式转换；
- 这是“可检查状态”类型的标准写法，`std::istream`、`std::unique_ptr` 都这么做。

### 3.2 其他转换运算符

写一个 `Meters` 或 `Fraction`，尝试：

```cpp
operator double() const;            // 隐式：容易出意外
explicit operator double() const;   // 显式：只能 static_cast / 直接初始化
```

回答：为什么大多数自定义类型不应提供隐式的 `operator double()`？什么时候显式转换是合理的？

### 3.3 构造函数方向的转换

转换既可以从“构造函数”来，也可以从“转换运算符”来：

```cpp
class Celsius {
public:
    Celsius(double degrees);              // double -> Celsius（隐式）
    explicit Celsius(int degrees);        // int -> Celsius（显式）
    explicit operator double() const;     // Celsius -> double（显式）
};
```

注意：单参数构造函数默认加 `explicit` 是防御性写法；只有确实希望隐式转换时才去掉（D04 已实测 `explicit` 阻止哪些写法）。

---

## 任务 4：lambda、`auto` 与 C++11 常用特性（约 70 min）

### 4.1 捕获方式

在 `code/lambda_demo.cpp` 里验证：

```cpp
int x = 10;

auto by_value = [x] { return x; };        // 值捕获：复制一份
auto by_ref   = [&x] { return ++x; };     // 引用捕获：改原变量
auto all_val  = [=] { return x; };        // 全部值捕获
auto all_ref  = [&] { return ++x; };      // 全部引用捕获
auto mutable_lambda = [x]() mutable { return ++x; };  // 改副本
```

要求记录：

- 值捕获后修改原变量，lambda 里看到的是旧值还是新值？
- 引用捕获的 lambda 在 `x` 生命周期结束后再调用会怎样？（悬垂引用，不要真的依赖结果）
- `mutable` 改的是哪一份 `x`？

### 4.2 初始化捕获（C++14）

```cpp
auto counter = [n = 0]() mutable { return ++n; };
```

- 捕获列表中直接创建新变量；
- 可以捕获 `std::move(obj)` 的结果，把对象移进 lambda：

```cpp
std::string text = "hello";
auto owns = [s = std::move(text)] { return s; };
```

### 4.3 泛型 lambda（C++14）

```cpp
auto twice = [](auto value) { return value + value; };
std::cout << twice(3) << ' ' << twice(std::string("ab")) << '\n';
```

说明：泛型 lambda 本质上是编译器生成的函数对象模板；模板细节 D7 再讲。

### 4.4 `auto`、range-for、`nullptr`、`enum class`

各写一个小例子：

```cpp
auto n = 42;                  // int
auto d = 3.14;                // double
auto p = nullptr;             // std::nullptr_t

std::vector<int> v{1, 2, 3};
for (const auto& item : v) { /* ... */ }

enum class Color { Red, Green, Blue };
Color c = Color::Red;
// if (c == 1) 错误：enum class 不隐式转 int
```

记录：`enum class` 和普通 `enum` 的区别（作用域、隐式转换）。

### 4.5 常见坑

- lambda 引用捕获局部变量，lambda 被存到作用域外 → 悬垂；
- `[=]` 在成员函数里捕获 `this` 的语义（C++20 已弃用隐式 `this` 捕获，建议写 `[this]` 或 `[*this]`）；
- 泛型 lambda 里 `auto&&` 与完美转发（D10）；
- 不要返回捕获了局部引用的 lambda。

---

## 任务 5：移动语义入门（约 80 min）

> D10 会系统讲右值引用、`std::forward`、引用折叠和异常安全；今天只要求“会写、会用、知道边界”。

### 5.1 左值 / 右值与 `std::move`

```cpp
int a = 1;
int&& r = std::move(a);   // std::move 只是把 a 转成右值引用
// a 仍然存在，r 绑定到 a
```

说明：`std::move(x)` **不移动任何东西**，它只是一个 `static_cast<T&&>`；真正的“移动”发生在移动构造 / 移动赋值里。

### 5.2 写一个 `Buffer`，实现 Rule of 5

在 `code/move_demo.cpp` 里写：

```cpp
class Buffer {
public:
    explicit Buffer(std::size_t size);
    ~Buffer();

    Buffer(const Buffer& other);
    Buffer& operator=(const Buffer& other);

    Buffer(Buffer&& other) noexcept;
    Buffer& operator=(Buffer&& other) noexcept;

    std::size_t size() const;
    int* data();
    const int* data() const;

private:
    int* data_;
    std::size_t size_;
};
```

要求：

- 构造 `new int[size]`，析构 `delete[]`；
- 拷贝构造 / 拷贝赋值深拷贝；
- 移动构造“偷指针”，把源对象置为 `nullptr` / `0`；
- 移动赋值先释放自己的资源，再偷源对象，并处理自赋值；
- 移动构造 / 移动赋值标 `noexcept`；
- 每个特殊成员打印一行，带对象地址或内容。

### 5.3 验证

```cpp
Buffer a(4);
Buffer b = a;              // 拷贝构造
Buffer c = std::move(a);   // 移动构造：a 被掏空
Buffer d(2);
d = std::move(c);          // 移动赋值：d 原资源释放，c 被掏空
```

记录输出，并检查：

- 移动后源对象的 `data()` 是否为 `nullptr`、`size()` 是否为 0；
- 被掏空的源对象析构时是否安全（`delete[] nullptr` 合法）；
- 移动赋值有没有泄漏 `d` 原来的内存（用 ASan 跑）。

### 5.4 为什么 `noexcept` 重要

写一个 `std::vector<Buffer>`，连续 `push_back` 触发扩容：

```cpp
std::vector<Buffer> v;
v.reserve(1);
for (int i = 0; i < 4; ++i) {
    v.emplace_back(1);
}
```

对比：

- 移动构造标 `noexcept` 时，扩容打印的是 `move`；
- 去掉 `noexcept` 后，扩容打印的是 `copy`。

解释：`vector` 需要强异常保证，移动可能抛异常时它会选择拷贝。

### 5.5 常见坑

- `return std::move(local);` 会阻止 NRVO，通常更差；
- 对 `const T` 调用 `std::move` 得到 `const T&&`，只能匹配拷贝构造；
- 移动后对象是“有效但未指定状态”，不要假设它为空（我们的实现里置空，但不是语言保证）；
- 移动构造不置空源指针 → double-free；
- 移动操作不标 `noexcept` → 容器退化为拷贝。

---

## 任务 6：C++14/17 标准库与语言特性（约 100 min）

新建 `code/features17.cpp`，每一项写一个可运行的小例子。

### 6.1 `std::make_unique`（C++14）

```cpp
auto p = std::make_unique<Vec3>(1, 2, 3);
std::cout << *p << '\n';
```

- `unique_ptr` 只能移动，不能拷贝；
- `make_unique` 避免裸 `new`，异常安全；
- D9 会系统讲智能指针的所有权模型。

### 6.2 structured bindings（C++17）

```cpp
std::pair<int, std::string> f();
auto [code, message] = f();

std::map<std::string, int> scores{{"a", 1}};
for (const auto& [name, score] : scores) { /* ... */ }
```

- 绑定到 `pair` / `tuple` / 公开成员的聚合；
- 引用绑定：`auto& [k, v] = ...`。

### 6.3 `if constexpr`（C++17）

```cpp
template <typename T>
std::string describe(const T& value) {
    if constexpr (std::is_arithmetic_v<T>) {
        return "number: " + std::to_string(value);
    } else {
        return "other";
    }
}
```

- 条件在编译期求值，未选中的分支不会实例化；
- 需要模板上下文才有意义；
- 和普通 `if` 的区别：普通 `if` 两边都要能编译。

### 6.4 `std::optional`（C++17）

```cpp
std::optional<int> parse_int(const std::string& s);

if (auto value = parse_int("42")) {
    std::cout << *value << '\n';
} else {
    std::cout << "invalid\n";
}
```

- `has_value()`、`operator bool`、`value()`、`value_or()`；
- `value()` 在空时抛 `std::bad_optional_access`；
- 适合“可能没有值”，但**不适合**表达详细错误原因（那用 `variant` / 异常 / `expected`）。

### 6.5 `std::variant`（C++17）

```cpp
std::variant<int, std::string> v = 42;
v = std::string("hello");

std::cout << std::holds_alternative<int>(v) << '\n';
std::cout << std::get<std::string>(v) << '\n';

std::visit([](const auto& x) { std::cout << x << '\n'; }, v);
```

- `std::get<T>` 类型不对会抛 `std::bad_variant_access`；
- `std::get_if<T>` 返回指针，失败返回 `nullptr`；
- `std::visit` + 泛型 lambda 是处理 variant 的常用组合；
- 了解 `valueless_by_exception()` 的存在即可。

### 6.6 `std::string_view`（C++17）

```cpp
void print(std::string_view sv) {
    std::cout << sv << " size=" << sv.size() << '\n';
}

std::string s = "hello";
print(s);          // 不拷贝
print("world");    // 不分配
```

- 非拥有视图，不管理内存；
- 不要返回指向局部字符串的 `string_view`；
- `data()` 不保证以 `'\0'` 结尾；
- 适合函数参数，不适合长期存储。

### 6.7 `std::filesystem`（C++17）

```cpp
namespace fs = std::filesystem;

fs::path p = "/tmp/d06";
fs::create_directories(p);
fs::path file = p / "demo.txt";

std::ofstream(file) << "hello\n";

for (const auto& entry : fs::directory_iterator(p)) {
    std::cout << entry.path() << '\n';
}

std::cout << fs::file_size(file) << '\n';
```

- GCC 9+ 已把 `<filesystem>` 并入 `libstdc++`，不需要额外 `-lstdc++fs`；GCC 8 及更早才需要；
- 练习 `path` 的拼接、`exists`、`is_regular_file`、`directory_iterator`；
- 不要在遍历目录时随意删除正在遍历的项。

### 6.8 `[[nodiscard]]`（C++17）

```cpp
[[nodiscard]] int compute();

compute();   // 警告：返回值被忽略
(void)compute();   // 显式忽略
```

- 用在“返回值必须处理”的函数上；
- 也可以标在类型 / 枚举上；
- 配合 `-Wall -Wextra` 观察警告。

### 6.9 fold expressions（C++17，预览）

```cpp
template <typename... Ts>
auto sum(const Ts&... values) {
    return (values + ...);   // 一元右折叠
}
```

- `(pack op ...)`、`(... op pack)`、`(init op ... op pack)` 三种形式；
- 本节只要求读懂并改一个简单例子，可变参数模板、折叠规则 D7 再展开。

### 6.10 其他值得知道的小特性

- 嵌套命名空间：`namespace a::b::c { }`；
- inline 变量（C++17）：头文件里定义 `inline constexpr` 变量；
- `std::byte`；
- `std::clamp`、`std::gcd` / `std::lcm`（`<numeric>`）；
- `if` / `switch` 初始化语句（C++17）：`if (auto it = m.find(k); it != m.end())`。

---

## 任务 7：`Vec3` 完整实现 + 特性示例集 + CMake + 笔记（约 60 min）

### 7.1 文件

```text
vec3.h
vec3.cpp
vec3_main.cpp
lambda_demo.cpp
move_demo.cpp
features17.cpp
```

`Vec3` 至少包含：

- 默认构造、三参构造；
- `x()` / `y()` / `z()`、`operator[]`；
- `+`、`-`、一元 `-`、`+=`、`-=`；
- `double * Vec3`、`Vec3 * double`；
- `==`、`!=`、`<`、`<=`、`>`、`>=`（通过 `<=>`）；
- `dot`、`length`；
- `operator<<`；
- 可选：`operator>>`、归一化、叉积。

要求：

- 头文件自包含，用 `#pragma once`；
- `.h` 里不 `using namespace std;`；
- 所有不修改对象的成员函数加 `const`；
- 零警告。

### 7.2 测试

`vec3_main.cpp` 覆盖：

```cpp
Vec3 a(1, 2, 3);
Vec3 b(4, 5, 6);
Vec3 c = a + b;
Vec3 d = 2.0 * a;
Vec3 e = a * 2.0;
a += b;
a -= b;
std::cout << a << '\n';
std::cout << (a == b) << ' ' << (a < b) << '\n';
std::cout << a.dot(b) << ' ' << a.length() << '\n';
```

再用 ASan + UBSan 跑一遍：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    vec3_main.cpp vec3.cpp -o /tmp/d06_vec3_asan
/tmp/d06_vec3_asan
```

要求无报错、退出码 0。

### 7.3 CMake

在 `base/CMakeLists.txt` 中加 target：

| target | 源文件 |
|---|---|
| `d06-vec3` | `vec3_main.cpp` + `vec3.cpp` |
| `d06-lambda` | `lambda_demo.cpp` |
| `d06-move` | `move_demo.cpp` |
| `d06-features` | `features17.cpp` |

要求：

- C++20 + `-Wall -Wextra -Wpedantic -Wconversion`，零警告；
- `fail_*.cpp` 不进 CMake；
- `d06-features` 用到 `<filesystem>`，GCC 13 不需要额外链接库。

```bash
cmake -S /mnt/d/Project/CPP/CPPLearn/base -B ~/build/base
cmake --build ~/build/base --target d06-vec3 d06-lambda d06-move d06-features -j"$(nproc)"
```

### 7.4 笔记

至少覆盖：

- 运算符重载的成员 / 非成员选择表；
- `operator<<`、`==`、`<=>` 的实现和输出；
- `explicit operator bool()` 允许 / 阻止的写法；
- lambda 捕获方式对比与悬垂风险；
- `Buffer` 的 Rule of 5 实现和移动前后状态；
- `vector` 扩容 `noexcept` 实验对比；
- C++14/17 每个特性的最小示例和输出；
- 3–5 条易错点。

按 `AGENTS.md` 的约定，每个知识点和实验后加 `> **记忆点：**`。

---

## 产出清单

- `code/vec3.h`、`vec3.cpp`、`vec3_main.cpp`；
- `code/lambda_demo.cpp`、`move_demo.cpp`、`features17.cpp`；
- 失败实验：`fail_explicit_conversion.cpp` 等（按需）；
- CMake 中 4 个新 target；
- `note_task*.md` 或一份 `note.md`；
- 至少一次 Git 提交，提交信息 `d06: 运算符重载与现代 C++`。

## 验收标准

- [ ] 能判断常见运算符该写成成员还是非成员，并说清对称性理由；
- [ ] `Vec3` 的 `+`、`-`、`+=`、`[]`、`<<`、`<=>` 全部正确，零警告；
- [ ] 能解释 `= default` 的 `<=>` 如何生成比较，以及 `double` 为什么是 `partial_ordering`；
- [ ] `explicit operator bool()` 能阻止 `bool b = c;`，但允许 `if (c)`；
- [ ] 能写出移动构造 / 移动赋值，源对象置空，ASan 无报错；
- [ ] 能解释 `std::move` 只是类型转换，以及为什么移动构造要 `noexcept`；
- [ ] 会用 `make_unique`、structured bindings、`if constexpr`、`optional`、`variant`、`string_view`、`filesystem`、`[[nodiscard]]`；
- [ ] 能读懂简单的 fold expression；
- [ ] 笔记完成，包含实测输出和记忆点。

## 自测问题

1. 哪些运算符必须是成员？`+` 和 `+=` 分别推荐哪种形式，为什么？
2. `operator+` 为什么常写成“按值接收左操作数 + 复用 `+=`”？
3. `operator<<` 的签名是什么？为什么必须是非成员？
4. `bool operator==(...) const = default;` 和 `auto operator<=>(...) const = default;` 分别生成什么？
5. 成员是 `double` 时，`<=>` 的返回类型是什么？为什么？
6. `explicit operator bool()` 允许哪些写法，阻止哪些写法？
7. lambda 的值捕获、引用捕获、初始化捕获有什么区别？悬垂引用怎么产生？
8. 泛型 lambda 的 `auto` 参数在编译期变成了什么？
9. `std::move` 做了什么？它本身会移动对象吗？
10. 移动构造为什么要把源对象的指针置空？不置空会发生什么？
11. 为什么移动构造常标 `noexcept`？不标时 `std::vector` 会怎样？
12. `optional`、`variant`、`string_view` 各自适合什么场景？
13. `if constexpr` 和普通 `if` 的本质区别是什么？
14. structured bindings 能绑定哪些类型？

## 完成后

告诉我「D06 完成」，并附上：

- `cmake --build` 的完整输出（确认零警告）；
- `Vec3` 的测试输出；
- `Buffer` 的移动前后输出、`vector` 扩容 `noexcept` 对比；
- `features17` 每个特性的实际输出；
- `note_task*.md` 或 `note.md`；
- `git log --oneline --graph`。

我会检查 `Vec3` 的运算符是否对称、`const` 是否正确、移动操作是否真的转移所有权且无泄漏；确认后再进入 D07（模板入门 + C++20 + CMake 工程化）。
