# D02 - 基础类型 + Linux 基础

## 今天解决的问题

- 任务 1（已完成）：
  - `<cstdint>` 固定宽度类型、`size_t`/`ptrdiff_t` 的宽度与范围；
  - `sizeof`/`alignof` 与 LP64/LLP64 数据模型；
  - 四种初始化、默认初始化的未定义值；
  - 列表初始化禁止窄化；
  - 有符号溢出 UB、无符号回绕、整型提升与混合符号比较；
  - 字符串字面量的类型与三种接收方式。
- 搞清 `types.cpp` 实现里的语法：匿名 namespace、`std::left`/`std::setw`、`if constexpr`、`std::numeric_limits`、`static_cast`、`static_assert`。

## 核心概念 & 为什么这样设计

### 基本类型与初始化（任务 1）

#### 固定宽度整数类型（`<cstdint>`）

- `std::int8_t` … `std::int64_t`、`std::uint8_t` … `std::uint64_t`：宽度精确为 N 位的整数别名（平台提供时才有）。适合协议、文件格式、序列化、硬件寄存器等需要确定宽度的场景；日常计数优先 `int`，大小/下标用 `std::size_t`。
- `int8_t`/`uint8_t` 通常是 `signed char`/`unsigned char` 的 typedef，不是独立类型。`std::cout << int8_var` 会按**字符**输出，要打印数值必须先转成更宽的整数。
- `char`、`signed char`、`unsigned char` 是**三种不同类型**；普通 `char` 是否有符号由实现决定（x86 Linux 上有符号）。
- `std::size_t`（`<cstddef>`）：`sizeof` 的结果类型，无符号；用于对象大小、数组下标、计数。x86-64 上 8 字节。
- `std::ptrdiff_t`（`<cstddef>`）：两个指针相减的结果类型，有符号。x86-64 上 8 字节。

实测（WSL Ubuntu 24.04 / g++ 13.3.0 / x86-64）：

```text
int8_t      sizeof = 1   min = -128, max = 127
int16_t     sizeof = 2   min = -32768, max = 32767
int32_t     sizeof = 4   min = -2147483648, max = 2147483647
int64_t     sizeof = 8   min = -9223372036854775808, max = 9223372036854775807
uint8_t     sizeof = 1   min = 0, max = 255
uint16_t    sizeof = 2   min = 0, max = 65535
uint32_t    sizeof = 4   min = 0, max = 4294967295
uint64_t    sizeof = 8   min = 0, max = 18446744073709551615
size_t      sizeof = 8   min = 0, max = 18446744073709551615
ptrdiff_t   sizeof = 8   min = -9223372036854775808, max = 9223372036854775807
```

#### `sizeof` 与 `alignof`

- `sizeof(T)` / `sizeof expr`：以**字节**为单位的大小，结果类型是 `std::size_t`；`sizeof(char) == 1` 由标准保证。`sizeof` 是编译期常量（C++ 没有 VLA），`sizeof expr` 通常不实际求值表达式。
- `alignof(T)`（C++11）：类型的**对齐要求**。对象的地址必须是 `alignof(T)` 的整数倍；它决定结构体填充和数组元素间距。
  - `alignof(char) == 1`，`alignof(int) == 4`，`alignof(double) == 8`（x86-64）。
  - 例：`struct S { char c; int i; };` 在 x86-64 上 `sizeof(S) == 8`：`c` 后填充 3 字节，保证 `i` 的地址是 4 的倍数。
- `sizeof` 描述对象表示的大小，不是取值范围：`sizeof(bool) == 1`，但 `bool` 只有 `true`/`false` 两种值。
- 常见误区：`sizeof(数组)` 是数组总字节数，但数组作为函数参数会退化为指针，函数内 `sizeof` 得到指针大小（任务 3 再展开）。

实测（同上环境）：

```text
bool            sizeof = 1   alignof = 1
char            sizeof = 1   alignof = 1
short           sizeof = 2   alignof = 2
int             sizeof = 4   alignof = 4
long            sizeof = 8   alignof = 8
long long       sizeof = 8   alignof = 8
float           sizeof = 4   alignof = 4
double          sizeof = 8   alignof = 8
long double     sizeof = 16  alignof = 16
void* (pointer) sizeof = 8   alignof = 8
size_t          sizeof = 8   alignof = 8
```

与 Windows x64 的对比（**预期值**；本机没有 Windows 编译器，未实测）：

| 类型            | Linux x86-64（LP64） | Windows x64（LLP64 / MSVC） | 说明                                          |
| ------------- | ------------------ | ------------------------- | ------------------------------------------- |
| `long`        | 8                  | **4**                     | Windows 为兼容 Win32 API 保留 32 位 `long`        |
| 指针            | 8                  | 8                         | 都是 64 位地址，无差异                               |
| `long double` | 16（x87 80 位扩展精度）   | **8**（等同 `double`）        | MSVC 直接把它映射成 `double`；MinGW-w64 x64 通常仍为 16 |

- 数据模型：Unix 类系统是 **LP64**（`long`、指针 64 位），Windows 是 **LLP64**（只有 `long long` 和指针 64 位）。跨平台代码不要假设 `long` 是 64 位；需要固定宽度就用 `<cstdint>`。
- `long double` 的精度/大小不可移植，标准只保证它至少和 `double` 一样。

#### 四种初始化

| 写法           | 名称    | 含义          | 注意                        |
| ------------ | ----- | ----------- | ------------------------- |
| `int a;`     | 默认初始化 | 不显式提供初值     | 函数内标量**不做初始化**，值不确定，读取 UB |
| `int b = 1;` | 拷贝初始化 | 用 `=` 初始化   | 类类型不会选择 `explicit` 构造函数   |
| `int c(1);`  | 直接初始化 | 用括号         | 类类型可以选择 `explicit` 构造函数   |
| `int d{1};`  | 列表初始化 | 用花括号（C++11） | **禁止窄化**；推荐默认使用           |

- `int a;` 的值取决于存储期和作用域：
  - 函数内局部变量：默认初始化，标量**不确定**（indeterminate），读取是 UB；
  - 全局/命名空间作用域或 `static` 局部变量：静态存储期，先零初始化，`a == 0`；
  - 类的非静态成员：由构造函数/成员初始化列表决定；标量成员没初始化时同样不确定。
- 想明确得到 0：`int a{};`（值初始化）或 `int a = 0;`。
- 初始化不是赋值：`int b = 1;` 是初始化；`b = 1;` 是赋值。对类类型，前者调用构造函数，后者调用 `operator=`。
- `int c();` 是**函数声明**（most vexing parse），不是变量定义；`int c(1);` 才是变量。花括号能避免这种歧义。
- `int d = {1};` 是拷贝列表初始化，同样禁止窄化。

#### 窄化（narrowing）

列表初始化禁止可能丢失信息的转换：

```cpp
int x{3.14};   // error: narrowing conversion of '3.14...' from 'double' to 'int' [-Wnarrowing]
char c{1000};  // error: narrowing conversion of '1000' from 'int' to 'char' [-Wnarrowing]
```

规则（简化）：

- **浮点 → 整数**：一律是窄化，哪怕值是 `3.0`；
- **整数 → 整数**：非常量一律是窄化；常量只有“值能放进目标类型”才允许；
- **浮点 → 更窄浮点**、**整数 → 浮点**：常量且能精确表示时才允许。

对照：

```cpp
int y = 3.14;   // 合法：隐式转换，向零截断为 3；-Wconversion 下警告 [-Wfloat-conversion]
char c = 1000;  // 合法，但 -Wall 下警告 [-Woverflow]：值变成 -24（signed char）
```

- 浮点 → 整数若值超出目标范围是 **UB**；整数 → 有符号整数超出范围的转换在 C++20 起按模 $2^N$ 定义（C++20 前是实现定义）。所以 `char c = 1000;` 即使能编译也不要写。
- 正确修法：确实要截断就显式 `static_cast<int>(value)` 并注释意图；要四舍五入用 `std::lround`；或者从一开始就用更宽/浮点类型。**不要**用 `(int)` 硬压来消警告。
- 列表初始化的价值：把“悄悄丢数据”变成编译错误。

#### 符号、范围与溢出

- 范围由 `<limits>` 的 `std::numeric_limits<T>` 给出；C++20 起有符号整数必须是**补码**表示，但补码表示**不代表**溢出有定义。
- **无符号溢出**：定义良好，按 $2^N$ 取模回绕：

```cpp
unsigned u = std::numeric_limits<unsigned>::max();
u += 1;  // u == 0
```

- **有符号溢出**：UB。

```cpp
int x = INT_MAX;
x += 1;  // UB
```

- 为什么是 UB：编译器可以假设有符号运算不溢出，从而做常量折叠、循环优化、分支消除；一旦真的溢出，行为不受标准约束。
- 不要依赖“实际跑出来是 `-2147483648`”；那只是某个编译器/优化级别下的巧合。`-fwrapv` 是 GCC 扩展，不是标准保证。
- **整型提升**：`int8_t`/`int16_t`/`bool`/`char` 参与算术时通常先提升为 `int`。
  - `int8_t a = 100, b = 100; auto c = a + b;` 中 `c` 是 `int`，值为 200，不会在 8 位处溢出。
- **混合符号**：`int` 与 `unsigned` 运算时，`int` 会先转成 `unsigned`，负数会变成很大的正数。

```cpp
int i = -1;
unsigned u = 1;
i < u;  // false：i 被转成 UINT_MAX，而不是 -1 < 1
```

- 这类比较在 C++ 下由 `-Wall`（`-Wsign-compare`）警告。
- 所以：涉及大小/下标用 `std::size_t`；涉及可能为负的差值用 `std::ptrdiff_t` 或更宽的有符号类型；不要把有符号和无符号混着比较。

### 匿名 namespace（只在本文件可见）

```cpp
namespace {

int helper() { return 1; }   // 只在当前 .cpp 内可见

}  // namespace

int main() {
    return helper();          // 同一个 TU 内不需要限定名
}
```

**是什么**

- `namespace { ... }` 是**匿名命名空间**（unnamed namespace），没有名字。
- C++11 起，其中声明的名字具有**内部链接**（internal linkage）：只在本翻译单元（一个 `.cpp` 编译出的 `.o`）内可见。
- 同一个 `.cpp` 内可直接用非限定名调用，不需要 `some_namespace::`。
- 注意区分“链接（linkage）”和“存储期（storage duration）”：这里说的是名字能否跨 `.o` 被其他 TU 找到，不是对象活多久。

**为什么 `types.cpp` 要包起来**

`print_limits`、`print_layout`、`fixed_width_types` 等只服务本文件。包进匿名 namespace 后：

1. **避免重名冲突**：如果另一个 `.cpp` 也定义了同名的 `print_layout`，不包会链接期报 multiple definition；包了则各自是不同实体，互不干扰。
2. **不污染全局符号表**：符号是本地的，其他 TU 看不到。
3. **给编译器/链接器更多优化空间**：可以内联、丢弃未使用的函数。
4. **函数、变量、类型、模板都能放**：`static` 只能用于函数和变量，不能用于类型/模板。

**包不包的区别**

- 单文件程序：运行结果没有区别，只是链接属性不同。
- 多文件链接：
  - 不包 → 外部链接（external linkage），同名同签名会撞车；
  - 包 → 内部链接，每个 TU 各有一份，不冲突。
- 可以用 `nm` 观察：

```bash
g++ -std=c++20 -c types.cpp -o types.o
nm -C types.o | grep print_limits
nm -C types.o | grep main
```

实际输出类似：

```text
0000000000000486 t void (anonymous namespace)::print_limits<signed char>(char const*)
0000000000000000 T main
```

- `t`（小写）= 本地 text 符号（local）；
- `T`（大写）= 全局 text 符号（global）。

**注意**

- `main` 必须在全局命名空间，所以它放在 `}  // namespace` 之后。
- **头文件里不要写匿名 namespace**：每个包含该头文件的 TU 都会得到一份独立实体，容易 ODR 违规、代码膨胀。头文件里用 `inline` 函数或具名 namespace。
- 旧写法 `static void helper();` 也能实现内部链接；匿名 namespace 是更现代、能容纳更多种类声明的写法。

### 字符串字面量：`const char*` / `char*` / `char[]`

C++20 中**<mark>普通窄字符串字面量 `"abc"` 的类型是 `const char[4]`（`'a','b','c','\0'`），具有静态存储期，内容不可修改（可能放在只读段，且相同字面量允许合并）。</mark>**

| 写法                       | 是否合法             | 变量类型          | 能否改内容                     | `sizeof`     | 说明                                                      |
| ------------------------ | ---------------- | ------------- | ------------------------- | ------------ | ------------------------------------------------------- |
| `const char* s = "abc";` | 合法               | `const char*` | `s[0] = 'x'` 不行；`s` 可以改指向 | 8（x86-64 指针） | 数组退化为指向字面量首字符的指针                                        |
| `char* bad = "abc";`     | **不合法**（C++11 起） | —             | —                         | —            | 丢弃 `const`；GCC 默认只报 warning，`-pedantic-errors` 下是 error |
| `char copy[] = "abc";`   | 合法               | `char[4]`     | `copy[0] = 'x'` 可以        | 4            | 复制出一份独立、可写的数组                                           |

**`const char* s = "abc";`**

- `"abc"` 是 `const char[4]`，在需要指针的上下文里**数组退化为** `const char*`。
- `s` 本身是一个指针变量，可以重新指向别的字面量：`s = "xyz";`。
- 不能通过 `s` 写：`s[0] = 'x';` 编译错误，因为指向的是 `const char`。
- 字面量有静态存储期，不随 `s` 离开作用域而销毁。

**`char* bad = "abc";` 为什么非法**

- 从 `const char[4]` 退化得到的是 `const char*`，再转成 `char*` 需要**去掉 const**；限定转换不能去 const，所以标准上 ill-formed。
- C++03 曾保留一条 deprecated 的隐式转换；C++11 把它移除了。
- GCC 为了兼容老代码，默认把它降级成 warning：

```text
warning: ISO C++ forbids converting a string constant to 'char*' [-Wwrite-strings]
```

加 `-pedantic-errors`（或 `-Werror=write-strings`）就是硬错误：

```text
error: ISO C++ forbids converting a string constant to 'char*' [-Wwrite-strings]
```

- 即使强行 `const_cast<char*>` 通过编译，写 `bad[0] = 'x';` 也是 **UB**：字面量可能在只读段，且可能与其他相同字面量共享存储。

**`char copy[] = "abc";`**

- 这是**数组初始化**，不是指针赋值：编译器把 `'a','b','c','\0'` 复制到 `copy` 的 4 个元素里。
- 类型是 `char[4]`，`sizeof(copy) == 4`；`copy` 是数组，不是指针，不能整体重新赋值（`copy = ...` 非法）。
- 内容可写：`copy[0] = 'A';`。
- 作为函数实参时会退化为 `char*`，此时函数内 `sizeof` 得到的是指针大小，不是数组大小。
- 想要只读的副本可以写 `const char copy[] = "abc";`。

**补充**

- `auto p = "abc";` 中 `auto` 会做数组到指针的退化，`p` 的类型是 `const char*`，不是 `const char[4]`。
- `decltype("abc")` 得到的是 `const char(&)[4]`（字符串字面量是左值），所以 `types.cpp` 里用 `std::remove_reference_t<decltype("abc")>` 才能得到 `const char[4]`。

### 任务 1 代码里的语法

#### `std::left` 与 `std::setw(12)`

```cpp
std::cout << std::left << std::setw(12) << name
          << "sizeof = " << std::setw(2) << sizeof(T) << ...
```

- 它们都是**流操纵符**（manipulator），靠 `operator<<` 的特有重载作用在流上。
- `std::left`（`<ios>`）：把对齐方式设为左对齐，**粘性**——设置后一直有效，直到改成 `std::right` / `std::internal`。
- `std::setw(n)`（`<iomanip>`）：设置**下一次**格式化输出的最小宽度为 `n`，只对紧接着的一个输出项有效，之后自动恢复为 0。超过 `n` 不截断。
- 所以 `std::left << std::setw(12) << name` 的效果是：`name` 左对齐、右侧补空格到 12 列。
- `std::setw` 的宽度包含输出内容本身；默认填充字符是空格，可用 `std::setfill` 改。

#### `if constexpr` 与 `std::is_signed_v<T>`

```cpp
template <typename T>
void print_limits(const char* name) {
    if constexpr (std::is_signed_v<T>) {
        // T 是有符号类型时，只实例化这一支
    } else {
        // T 是无符号类型时，只实例化这一支
    }
}
```

- `if constexpr` 是 **C++17** 的编译期 `if`，条件必须是常量表达式。
- 在模板里，未被选中的分支是 **discarded statement，不会实例化**。这正是它和普通 `if` 的关键区别：普通 `if` 要求两个分支都能编译；`if constexpr` 可以写“只对某些 T 成立”的代码。
- 非模板函数里也允许 `if constexpr`，但此时被丢弃的分支仍然会被完整检查，**不能当 `#if` 用**。
- `std::is_signed_v<T>` 是 `<type_traits>` 里的**变量模板**（C++17），等价于 `std::is_signed<T>::value`；对 `int8_t` 为 true，对 `uint8_t` 为 false。
- 小陷阱：`std::is_signed_v<float>` 也是 true（浮点也算 signed）。这里只实例化整数类型，所以没问题。
- 本例用它决定按 `long long` 还是 `unsigned long long` 打印，避免把 `uint64_t` 的大值按有符号解释。

#### `std::numeric_limits<T>`

```cpp
std::numeric_limits<T>::min()
std::numeric_limits<T>::max()
```

- `<limits>` 中的类模板，为算术类型提供特化；成员是 `static constexpr`。
- 对**整数**：`min()` 是最小值，`max()` 是最大值。
- 对**浮点**：`min()` 是**最小正正规数**，不是最负值！要最负的有限值用 `lowest()`（C++11）。
- 其他常用成员：`is_signed`、`is_integer`、`digits`、`epsilon()`、`infinity()`、`is_specialized`。
- `std::numeric_limits<std::int8_t>::min()` 返回的类型是 `int8_t`（本质是 `signed char`），直接交给 `std::cout` 会被当成字符打印，所以要先转成数值类型。

#### `static_cast<long long>(...)`

- **显式类型转换**，编译期做类别检查；比 C 风格 `(long long)x` 更窄、更安全、可读、可搜索。
- `static_cast` 能做的：算术类型互转、`void*` 与对象指针互转、基类/派生类指针或引用互转、调用 explicit 构造/转换函数。
- `static_cast` 不能做的：去掉 `const`（用 `const_cast`）、无关指针类型互转（用 `reinterpret_cast`）、一次完成多种转换。
- 这里 `int8_t -> long long` 是**值保持的整型加宽**，目的只是让 `operator<<` 选择整数重载而不是字符重载；源值一定能被 `long long` 精确表示，所以不是“用强转掩盖问题”。
- 反例：`uint64_t` 的值可能大于 `LLONG_MAX`，此时 `static_cast<long long>` 不是值保持的，应改用 `unsigned long long`——这就是 `if constexpr` 分两支的原因。

#### `static_assert`

```cpp
static_assert(sizeof("abc") == 4);
static_assert(std::is_same_v<std::remove_reference_t<decltype("abc")>,
                             const char[4]>);
```

- **编译期断言**，条件必须是常量表达式；C++17 起可以省略消息，写成 `static_assert(cond);`。
- 条件为假时直接编译错误，不生成任何运行时代码。
- 和 `<cassert>` 的 `assert` 不同：后者是运行时检查，定义 `NDEBUG` 后可能被去掉。
- `decltype("abc")` 是 `const char(&)[4]`（字符串字面量是左值），`std::remove_reference_t` 去掉引用后得到 `const char[4]`，再和 `const char[4]` 比较。

### const / constexpr / auto

（任务 2 待补）

### 引用与指针

（任务 3 待补）

### 左值 / 右值

（任务 4 待补）

### 进程与信号

（任务 5 待补）

## 易错点 / UB（附最小复现代码）

### 窄化

```cpp
int x{3.14};   // error: narrowing conversion of '3.14...' from 'double' to 'int' [-Wnarrowing]
char c{1000};  // error: narrowing conversion of '1000' from 'int' to 'char' [-Wnarrowing]
int y = 3.14;  // 只是隐式转换；-Wconversion 下 warning: [-Wfloat-conversion]
```

- 浮点 → 整数在列表初始化里**一律**算窄化，哪怕值是 `3.0`。
- 整数 → 更窄整数：常量表达式只有“值能放进目标类型”才允许。
- 列表初始化的价值：把“悄悄丢数据”变成编译错误。

真实报错（g++ 13.3.0）：

```text
fail_narrowing.cpp:10:11: error: narrowing conversion of ‘3.1400000000000001e+0’ from ‘double’ to ‘int’ [-Wnarrowing]
fail_narrowing.cpp:11:12: error: narrowing conversion of ‘1000’ from ‘int’ to ‘char’ [-Wnarrowing]
```

`-Wconversion` 对隐式 `double -> int` 的警告：

```text
warn_conversion.cpp:8:13: warning: conversion from ‘double’ to ‘int’ may change value [-Wfloat-conversion]
```

### 有符号溢出

```cpp
int x = INT_MAX;
x += 1;  // UB
```

UBSan（`-fsanitize=undefined -fno-sanitize-recover=all`）输出：

```text
types.cpp:110:7: runtime error: signed integer overflow: 2147483647 + 1 cannot be represented in type 'int'
```

（行号随代码编辑变化。）退出码为 1；`-fno-sanitize-recover=all` 让 UBSan 报告后立即终止，后面的代码不会执行。

**不要**把 UB 当“结果可预测”：去掉 `-fno-sanitize-recover` 后可能打印 `-2147483648`，那只是某次编译/某个优化级别下的巧合。编译器有权假设有符号溢出不会发生。

无符号则不同：

```cpp
unsigned u = std::numeric_limits<unsigned>::max();
u += 1;  // 定义良好，u == 0
```

标准保证无符号运算按 $2^N$ 取模回绕。

> 演示时用命令行 `-DUB_DEMO` 控制，不要在源码里写 `#define UB_DEMO 1`，否则普通构建也会执行 UB。

### 整型提升与混合符号

```cpp
int i = -1;
unsigned u = 1;
std::cout << (i < u);  // 0（false）
```

- `int` 和 `unsigned` 比较时，`int` 先转成 `unsigned`；`-1` 变成 `UINT_MAX`。
- C++ 下 `-Wall` 会开启 `-Wsign-compare`。不要用强转消警告；先想清楚这个比较在语义上该不该发生。
- `int8_t`/`int16_t` 的算术会先提升为 `int`，所以 `int8_t + int8_t` 不会在 8 位处回绕。

### 普通 `char` 的符号性

```cpp
char c = 200;  // 在有符号 char 的平台上得到 -56
```

- `char` 是否有符号由实现决定（x86 Linux 有符号，ARM 上常见无符号）。
- 存数字字节用 `std::uint8_t` / `unsigned char`，不要用裸 `char` 当数值类型。
- `signed char`、`unsigned char`、`char` 是三种不同的类型。

### 字符串字面量

```cpp
char* bad = "abc";  // 不合法：const char[4] -> char* 丢弃 const
```

- GCC 默认 warning：`ISO C++ forbids converting a string constant to 'char*' [-Wwrite-strings]`
- 加 `-pedantic-errors` 或 `-Werror=write-strings` 才是硬错误。
- 即使强转通过编译，写 `bad[0] = 'x';` 也是 UB（只读段/字面量合并）。

### 匿名 namespace 放头文件

头文件里写匿名 namespace，会导致每个包含它的 TU 各有一份不同实体，容易 ODR 违规和代码膨胀。头文件用 `inline` 函数或具名 namespace；匿名 namespace 放 `.cpp`。

## 代码片段

### 构建命令（任务 1）

```bash
# 普通构建：要求零警告
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror types.cpp -o types

# UB 演示：单独构建
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -DUB_DEMO -fsanitize=undefined -fno-sanitize-recover=all -g types.cpp -o types_ub

# 故意失败的例子：不要加进构建系统
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -c fail_narrowing.cpp
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -pedantic-errors -c fail_string_literal.cpp
```

- 我用 `-Werror` 验证过：普通构建和 `-DUB_DEMO` 构建都是 **exit 0、零警告**。
- `-Werror` 只用来验证零警告；日常开发可以先不加，方便逐条看。
- `-fno-sanitize-recover=all` 让 UBSan 报告后立即终止；去掉它会继续执行，但 UB 结果依然不可依赖。

### 三种字符串写法

```cpp
const char* s = "abc";  // 指向字面量；s 可改指向，s[0] 不可改
// char* bad = "abc";   // 非法：丢弃 const
char copy[] = "abc";    // 复制一份；copy[0] 可改，sizeof(copy) == 4
copy[0] = 'A';
std::cout << s << ' ' << copy << '\n';  // abc Abc
```

### 匿名 namespace + 查看符号

```cpp
namespace {
void helper() {}
}

int main() { helper(); }
```

```bash
g++ -std=c++20 -c demo.cpp -o demo.o
nm -C demo.o | grep helper    # t void (anonymous namespace)::helper()
```

小写 `t` 表示本地符号，说明它不会被其他 `.o` 看见。

## 待复习

- `int8_t` 与 `char` 的关系；为什么 `std::cout << int8_var` 会打印字符。
- `size_t` 与 `ptrdiff_t` 分别解决什么问题；为什么 `size_t` 是无符号的。
- `sizeof` 与 `alignof` 的区别；结构体为什么需要填充；LP64 与 LLP64 的差异。
- 四种初始化；默认初始化、值初始化、静态存储期零初始化的区别。
- 窄化的三条规则；`int x{3.0}` 为什么失败；如何正确修掉 `-Wconversion`。
- 有符号溢出与无符号回绕的区别；为什么不能依赖 UB 的结果。
- 整型提升与 `int`/`unsigned` 混合比较的坑（`-1 < 1u`）。
- 匿名 namespace 与 `static` 的异同；为什么不能放头文件。
- `std::setw` 为什么只对下一次输出有效，`std::left` 为什么是粘性的。
- `if constexpr` 和普通 `if` 的区别；discarded statement 什么时候不实例化。
- `std::numeric_limits` 对浮点时 `min()` 的陷阱（不是最负值，用 `lowest()`）。
- `static_cast` 与 C 风格转换的区别；哪些转换它做不了。
- `static_assert` 与 `assert` 的区别。
- `const char*` / `char*` / `char[]` 接字符串字面量时的类型、可写性、`sizeof`。
