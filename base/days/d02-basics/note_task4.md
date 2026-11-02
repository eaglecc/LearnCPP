# D02 任务 4：左值 / 右值（初步）

> 任务 4 只要求建立直觉，不要求写复杂代码；本篇是笔记，没有新增仓库内的 `.cpp` 文件。
> 下面的验证片段是用 `/tmp` 下的小文件编译运行的，不影响项目构建。

## 一句话总结

- **每个表达式都有两个属性**：类型（type）和值类别（value category）。
- **左值**：有身份（identity），通常是“有名字、能持续存在”的对象，例如变量、`*p`、`arr[0]`、`++x`。
- **右值**：临时对象、字面量、运算结果，例如 `42`、`x + 1`、`x++`；通常不能取地址。
- **右值引用 `T&&`** 用来绑定右值，是移动语义的语法基础；左值引用 `T&` 绑定左值。
- **`std::move` 本身不移动任何东西**：它只是把表达式转换成右值引用（`T&&`），真正搬资源的是随后被选中的移动构造/移动赋值。

## 1. 为什么需要“值类别”

同一个类型 `int`，在不同表达式里可能是左值，也可能是右值：

```cpp
int x = 1;

f(x);   // x 是左值
f(42);  // 42 是右值
```

编译器可以据此选择不同的重载：

```cpp
#include <iostream>

void f(int&) {std::cout << "lvalue\n";}
void f(int&&) { std::cout << "rvalue\n"; }

int main() {
    int x = 1;
    f(x);   // lvalue
    f(42);  // rvalue
}
```

实际输出：

```text
lvalue
rvalue
```

这就是右值引用存在的直接原因：**让函数能够区分“调用方还要继续使用这个对象”和“这个对象马上就要销毁，可以安全地搬走资源”**。移动构造、移动赋值、完美转发都建立在这个区分上，D6/D10 再展开。

## 2. 三种值类别

C++11 起，值类别主要分三种：

| 类别               | 有身份（identity） | 典型例子                                   | 能否用内置 `&` 取地址           |
| ---------------- | ------------- | -------------------------------------- | ----------------------- |
| **lvalue**（左值）   | 有             | `x`、`*p`、`arr[0]`、`++x`、`"abc"`        | 通常可以（位域等例外）             |
| **prvalue**（纯右值） | 无             | `42`、`x + 1`、`x++`、`std::string("a")`  | 不可以                     |
| **xvalue**（将亡值）  | 有             | `std::move(x)`、`static_cast<int&&>(x)` | 内置 `&` 仍不接受（它要求 lvalue） |

- **lvalue**：有身份，能区分“是不是同一个对象”；通常能取地址，能出现在赋值左边（前提是 **modifiable**，见下面的坑）。
- **prvalue**：纯右值，没有身份，是临时值；不能用 `&` 取地址。
- **xvalue**：把左值“标记为即将销毁”后得到的结果，有身份，但允许被移动；`std::move` 产生 xvalue。
- **glvalue** = lvalue + xvalue（有身份的那一类）；**rvalue** = prvalue + xvalue（可以被移动的那一类）。

> “有身份”不等于“能赋值”。`const int cx = 1;` 中 `cx` 是左值，但不能赋值；数组名也是左值，但不能整体赋值。任务里说“左值能出现在赋值左边”，准确说法是 **modifiable lvalue** 才能被赋值。

### 字符串字面量是左值

这点容易反直觉：`"abc"` 是字面量，但它的类型是 `const char[4]`，**是左值**，不是 prvalue。

```cpp
static_assert(std::is_same_v<decltype(&"abc"), const char (*)[4]>);
```

- `&"abc"` 合法，得到“指向 `const char[4]` 的指针”，不是 `const char**`。
- 但它是 `const` 的，内容不能改：`"abc"[0] = 'A'` 非法。
- 所以“字面量都是右值”是错的；准确说：**数值字面量是 prvalue，字符串字面量是 lvalue**。

### `++x` 与 `x++`

```cpp
static_assert(std::is_same_v<decltype(++x), int&>);  // 前置 ++ 返回左值
static_assert(std::is_same_v<decltype(x++), int>);   // 后置 ++ 返回右值（旧值的副本）
```

- `++x` 修改 `x` 并返回 `x` 本身，所以是左值。
- `x++` 返回修改前的临时副本，所以是 prvalue。

## 3. 左值引用与右值引用

| 引用类型         | 能绑定什么              | 例子                    | 用途            |
| ------------ | ------------------ | --------------------- | ------------- |
| `int&`       | 非 const 左值         | `int& r = x;`         | 修改调用方对象       |
| `const int&` | 左值、右值、临时对象         | `const int& cr = 42;` | 只读；可以接临时值     |
| `int&&`      | 右值（prvalue/xvalue） | `int&& rr = 42;`      | 移动语义、完美转发（后面） |

### 左值引用

```cpp
int x = 1;
int& r = x;   // r 是 x 的别名

r = 2;        // 改的是 x
```

- `r` 不是独立对象，`&r == &x`。
- 引用必须在定义时绑定，之后不能改绑；`r = y` 是给 `x` 赋值，不是让 `r` 去引用 `y`。
- `int& r = 42;` 非法：非 const 左值引用不能绑定右值。

g++ 13.3.0 的实际报错：

```text
error: cannot bind non-const lvalue reference of type 'int&' to an rvalue of type 'int'
```

### const 左值引用可以绑定右值

```cpp
const int& cr = 42;   // OK：临时对象的生命周期延长到 cr 的作用域结束
```

- 这是 `const T&` 的特殊能力；函数参数 `const std::string&` 能接字面量和临时 `std::string`，原因就在这里。
- 生命周期延长只针对“直接绑定到引用”的临时对象，不要把引用返回出去，具体规则 D6/D10 再展开。

### 右值引用

```cpp
int&& rr = 42;   // OK：绑定到临时对象，生命周期延长
rr = 43;         // OK：rr 是具名变量，本身是左值
int* p = &rr;    // OK：能取地址
```

实际输出（地址每次运行会变）：

```text
x=1 r=1 rr=42 cr=7
&x=0x7ffde33d076c &r=0x7ffde33d076c &rr=0x7ffde33d0770
after rr=43: rr=43
```

关键点：

- `rr` 的**声明类型**是 `int&&`（右值引用），但 `rr` 是一个**具名变量**，所以表达式 `rr` 的值类别是 **lvalue**。
- 因此 `rr = 43`、`&rr` 都合法；`decltype(rr)` 仍是 `int&&`，但 `decltype((rr))` 是 `int&`。
- 这也是为什么在函数 `void f(T&& t)` 里，`t` 本身是左值；要把“右值身份”继续传下去，需要 `std::move(t)`（D6 的完美转发再细讲）。

```cpp
static_assert(std::is_same_v<decltype(rr), int&&>);
static_assert(std::is_same_v<decltype((rr)), int&>);
static_assert(std::is_same_v<decltype(std::move(x)), int&&>);
```

### 右值引用不能绑定左值

```cpp
int x = 1;
int&& rr = x;   // 错误：x 是左值
```

实际报错：

```text
error: cannot bind rvalue reference of type 'int&&' to lvalue of type 'int'
```

如果确实想把 `x` 当作“可以搬走”的对象，要显式转换：

```cpp
int&& rr = std::move(x);              // xvalue
int&& rr2 = static_cast<int&&>(x);    // 等价的手写转换
```

### 右值也不能随便取地址

```cpp
int* p = &42;            // 错误
int* q = &std::move(x);  // 错误：xvalue 也不是 lvalue
```

g++ 13.3.0 的实际报错：

```text
error: lvalue required as unary '&' operand
```

```text
error: taking address of rvalue [-fpermissive]
```

- `&` 要求操作数是 **lvalue**；prvalue 不行，xvalue 也不行（尽管 xvalue 有身份）。
- 这也是为什么“右值通常不能取地址”里的“通常”要保留：xvalue 有身份，只是内置 `&` 不接受它。

## 4. `std::move` 到底做了什么

一句话：

> **`std::move(x)` 等价于 `static_cast<std::remove_reference_t<decltype(x)>&&>(x)`：它只做一次到右值引用的类型转换，不读取、不修改、不搬运任何资源；真正搬资源的是随后被选中的移动构造/移动赋值。**

验证：

```cpp
#include <string>
#include <utility>

std::string s = "hello";
std::cout << "before move: s=" << s << ", size=" << s.size() << '\n';

std::string&& sr = std::move(s);   // 只是转换，s 没变
std::cout << "after std::move(s) alone: s=" << s << ", size=" << s.size() << '\n';

std::string t = std::move(sr);     // 这里才调用移动构造，搬走缓冲区
std::cout << "after t=std::move(sr): s=" << s << ", size=" << s.size()
          << ", t=" << t << '\n';
```

libstdc++ 上的实际输出：

```text
before move: s=hello, size=5
after std::move(s) alone: s=hello, size=5
after t=std::move(sr): s=, size=0, t=hello
```

- 只写 `std::move(s);` 不会改变 `s`，也不会发生任何移动；它只是一个被丢弃的转换结果。
- `std::string t = std::move(sr);` 才会匹配 `std::string(std::string&&)` 移动构造函数，把 `s` 的缓冲区“偷”给 `t`。
- 被移动后的 `s` 处于“**有效但未指定**”的状态（valid but unspecified）：标准不保证它为空；libstdc++ 这里恰好为空，但不能依赖。继续使用前应重新赋值或 `clear()`。

### `std::move` 不保证一定移动

- 对 `int` 这种内置类型，“移动”就是拷贝，没有资源可偷。
- 如果类型没有移动构造函数，或者移动构造不可用，编译器会退回复制。
- 对 `const` 对象使用 `std::move`，得到的是 `const T&&`，通常无法绑定到 `T&&` 移动构造，于是走拷贝：

```cpp
const std::string cs = "hello";
std::string t = std::move(cs);   // 实际调用拷贝构造
```

实际输出：

```text
cs=hello, t=hello
```

- 所以“写了 `std::move` 就一定会移动”是错的；它只是**请求**按右值处理，最终由重载决议决定。

## 5. 常见坑

1. **左值 ≠ 一定能赋值**：`const int cx = 1;` 的 `cx` 是左值但不能赋值；数组名是左值但不能整体赋值。能赋值的是 **modifiable lvalue**。
2. **`int& r = 42;` 非法**：非 const 左值引用不能绑定右值；用 `const int&` 或 `int&&`。
3. **`int&& rr = x;` 非法**：右值引用不能绑定左值；确实要“视作右值”时用 `std::move(x)`。
4. **具名右值引用是左值**：`int&& rr = 42;` 之后，表达式 `rr` 是 lvalue；在函数里继续传递时要 `std::move(rr)`，否则会选择拷贝。
5. **`std::move` 本身不移动**：`std::move(s);` 单独一行什么也不会发生；必须把它用于构造、赋值或函数实参。
6. **移动后源对象状态未指定**：不要假设它一定为空，也不要继续读它的值；重新赋值或 `clear()` 后再用。
7. **`std::move` 对 const 对象通常变成拷贝**：得到 `const T&&`，移动构造一般接不了。
8. **字符串字面量是左值**：`"abc"` 的类型是 `const char[4]`，`&"abc"` 合法；不要把它当 prvalue。
9. **xvalue 也不能用 `&` 取地址**：`&std::move(x)` 报错；`&` 只接受 lvalue。
10. **`std::move` 不等于 `std::forward`**：`move` 是无条件转成右值；`forward` 按原值类别转发，D6/D10 再学。

## 6. 待复习

- 表达式的两个属性：类型与值类别。
- lvalue / prvalue / xvalue 的区别；glvalue 与 rvalue 分别包含谁。
- “有身份”与“能赋值”的区别；为什么 `const` 左值和数组不能赋值。
- 字符串字面量为什么是左值；`&"abc"` 的类型。
- `++x` 与 `x++` 的值类别和 `decltype` 结果。
- `int&`、`const int&`、`int&&` 各自能绑定什么。
- 为什么 `int& r = 42` 和 `int&& rr = x` 都非法；怎么改。
- 具名右值引用为什么是左值；`decltype(rr)` 与 `decltype((rr))` 的区别。
- `std::move` 的实现本质；为什么它本身不移动；移动后源对象的状态。
- `std::move` 对 `const` 对象为什么通常变成拷贝。
