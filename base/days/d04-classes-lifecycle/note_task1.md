# D04 任务 1：构造、析构、初始化列表、`explicit`

> 环境：WSL2 Ubuntu 24.04，g++ 13.3.0，CMake 3.28.3。
> 代码目录：[code/](code/)
> 任务 1 的代码已补齐，下面的输出和报错都来自实际编译、运行；故意失败的文件只用来记录错误，不进 CMake。

## 0. 速查

- 初始化列表是**直接构造成员**；函数体里赋值是**先默认构造、再赋值**。
- `const` 成员和引用成员必须在初始化列表里“出生时就定型”；函数体里对 `const` 赋值编译失败，对引用赋值只是写穿，不能改绑。
- 成员初始化顺序由**声明顺序**决定，和初始化列表的书写顺序无关；析构顺序相反。
- 局部对象按定义顺序构造，离开作用域时逆序析构；内层作用域先析构。
- 成员对象先于对象自身的构造函数体构造，晚于对象自身的析构函数体析构。
- `explicit` 阻止的是**隐式转换 / 复制初始化**，不阻止直接初始化、`static_cast`、`T{...}`。
- 有函数体的成员函数定义 `{}` 后面**不需要**分号；只有声明、`= default`、`= delete` 和成员变量需要分号。多写一个 `;` 是空声明，合法但多余。
- 类内 `Point();` 只是**声明**，本身不初始化任何东西；`Point() = default;` 是**定义**，但也不会把 `int` 清零。
- `int` 成员不会自动为 0；要 0 就写初始化列表或默认成员初始化器。

## 1. 文件清单

| 文件 | 内容 | 结果 |
|---|---|---|
| `ctors.cpp` | `Point`、`Config`、`Trace`、`Holder`、`NonCopyable` + `main` | 编译运行成功，零警告 |
| `fail_const_assign.cpp` | 在构造函数体里给 `const` 成员赋值 | 编译失败（预期） |
| `fail_ref_assign.cpp` | 在构造函数体里给引用成员“赋值” | 编译失败（预期） |
| `warn_reorder.cpp` | 初始化列表顺序与成员声明顺序相反 | 触发 `-Wreorder` / `-Wuninitialized` |
| `fail_explicit.cpp` | `explicit` 阻止 `Box b = 10`、`take(10)` | 加 `-DEXPLICIT` 时编译失败 |
| `fail_explicit_variants.cpp` | `explicit` 与直接 / 复制初始化的各种写法 | 4 处编译错误（预期） |
| `fail_noncopyable.cpp` | 拷贝、赋值被 `= delete` 的类 | 编译失败（预期） |
| `fail_noncopyable_no_default.cpp` | 只写 `= delete` 拷贝构造、不补默认构造 | 编译失败（预期） |
| `fail_ctor_undefined.cpp` | 默认构造只声明、不定义 | 链接失败（预期） |
| `fail_missing_semicolon.cpp` | `= default` 后漏分号 | 编译失败（预期） |
| `default_init.cpp` | 声明 / `= default` / 初始化列表 / 值初始化对照 | 编译运行成功 |

## 2. 构建与运行

```bash
cd base/days/d04-classes-lifecycle/code
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    ctors.cpp -o /tmp/d04_ctors
/tmp/d04_ctors
```

实际输出：

```text
== Point ==
Point(0, 0)
Point(3, 4)

== Config ==
Config id=42, value=10
v after cfg.value() = 99: 99

== Trace: local objects and nested scope ==
Trace ctor: a
Trace ctor: b
Trace ctor: c
Trace dtor: c
Trace ctor: d
Trace dtor: d
Trace dtor: b
Trace dtor: a

== Holder: member objects ==
Trace ctor: holder.first
Trace ctor: holder.second
Holder ctor
Holder dtor
Trace dtor: holder.second
Trace dtor: holder.first

== NonCopyable ==
NonCopyable constructed
```

带 `-Werror` 仍然零警告，说明初始化列表、多写的分号、`= default` / `= delete` 这些写法都没有告警问题。

> **记忆点：** 构造顺序看“声明 / 定义顺序”，析构顺序永远反过来；初始化列表只决定“用哪个值构造”，不决定“谁先构造”。

## 3. 构造函数与初始化列表

### 3.1 `Point`

```cpp
class Point {
public:
    Point();                                    // 只声明，定义在类外
    Point(int x, int y) : x_(x), y_(y) {};      // 定义在类内
    void print() const;

private:
    int x_;
    int y_;
};

Point::Point() : x_(0), y_(0) {}                // 类外定义
```

`Point origin;` 调用默认构造，因为定义里写了 `: x_(0), y_(0)`，输出 `Point(0, 0)`；`Point p(3, 4);` 用初始化列表，输出 `Point(3, 4)`。

如果把默认构造写成 `Point::Point() {}`，`x_`、`y_` 就是**默认初始化**的 `int`，值不确定，不是 0。这一点在第 8 节用 `default_init.cpp` 实测。

> **记忆点：** `int` 成员不会因为“在类里”就自动为 0；默认构造要清零，必须自己写 `x_(0), y_(0)` 或 `int x_ = 0;`。

### 3.2 `Config`：`const` 成员与引用成员

```cpp
class Config {
public:
    Config(int id, int& value) : id_(id), value_(value) {}

    int id() const { return id_; }
    int& value() const { return value_; }

private:
    const int id_;
    int& value_;
};
```

`const` 成员 `id_` 只能在初始化列表里初始化。在函数体里写 `id_ = id;`，实际报错：

```text
fail_const_assign.cpp: In constructor ‘Config::Config(int, int&)’:
fail_const_assign.cpp:4:5: error: uninitialized const member in ‘const int’ [-fpermissive]
    4 |     Config(int id, int& value) : value_(value) {
      |     ^~~~~~
fail_const_assign.cpp:9:15: note: ‘const int Config::id_’ should be initialized
fail_const_assign.cpp:5:13: error: assignment of read-only member ‘Config::id_’
```

两条错误分别说明两件事：构造函数的初始化阶段没有给 `id_` 值；函数体里的 `id_ = id` 又想改只读成员。

引用成员 `value_` 同理，必须先绑定。在函数体里写 `v_ = v;` 报错：

```text
fail_ref_assign.cpp: In constructor ‘RefOnly::RefOnly(int&)’:
fail_ref_assign.cpp:6:5: error: uninitialized reference member in ‘int&’ [-fpermissive]
    6 |     RefOnly(int& v) {
      |     ^~~~~~~
fail_ref_assign.cpp:11:10: note: ‘int& RefOnly::v_’ should be initialized
```

注意引用和 `const` 的失败原因不完全一样：`const` 是“不能写”，引用是“根本没有绑定”。即使引用已经通过初始化列表绑定，函数体里的 `value_ = value` 也只是**写穿到被引用的对象**，不会让引用改绑到另一个对象——引用一旦绑定就不能换目标。

`main` 里 `cfg.value() = 99;` 之后外部变量 `v` 也变成 99，输出：

```text
Config id=42, value=10
v after cfg.value() = 99: 99
```

`value()` 是 `const` 成员函数，但它返回的是引用成员绑定的那个 `int&`，所以可以写穿。

> **记忆点：** `const` 成员“不能赋值”，引用成员“不能改绑”；两者都只能在初始化列表里一次成型，函数体里已经晚了。

### 3.3 初始化列表 vs 函数体赋值

```cpp
Point::Point(int x, int y) : x_(x), y_(y) {}   // 初始化
Point::Point(int x, int y) { x_ = x; y_ = y; } // 赋值
```

对 `int` 成员，两种写法最终结果一样，所以容易误以为没区别。真正的区别是：

| 写法 | 成员实际经历的过程 | 适用性 |
|---|---|---|
| 初始化列表 | 用给定值直接构造成员 | `const`、引用、没有默认构造的类类型都可以 |
| 函数体赋值 | 先默认构造，再调用赋值运算符 | `const`、引用、没有默认构造的类类型都不可行 |

即使对能编译的类型，函数体赋值也可能多一次“默认构造 + 赋值”的开销。所以成员初始化优先写初始化列表。

> **记忆点：** 初始化列表是“直接出生”，函数体赋值是“先出生再改”；`const`、引用、无默认构造的成员只有前者能活。

## 4. 构造与析构顺序

### 4.1 局部对象与嵌套作用域

```cpp
{
    Trace a("a");
    Trace b("b");
    {
        Trace c("c");
    }
    Trace d("d");
}
```

实际输出：

```text
Trace ctor: a
Trace ctor: b
Trace ctor: c
Trace dtor: c
Trace ctor: d
Trace dtor: d
Trace dtor: b
Trace dtor: a
```

- 局部对象按定义顺序构造：`a`、`b`、`c`、`d`。
- 离开作用域时逆序析构：内层的 `c` 先析构，然后 `d`、`b`、`a`。
- `d` 在 `c` 的作用域结束之后才构造，所以 `Trace ctor: d` 出现在 `Trace dtor: c` 之后。

> **记忆点：** 局部对象像栈：后构造的先析构；内层作用域先于外层清理。

### 4.2 成员对象

```cpp
class Holder {
public:
    Holder() : first_("holder.first"), second_("holder.second") {
        std::cout << "Holder ctor\n";
    }
    ~Holder() { std::cout << "Holder dtor\n"; }

private:
    Trace first_;
    Trace second_;
};
```

实际输出：

```text
Trace ctor: holder.first
Trace ctor: holder.second
Holder ctor
Holder dtor
Trace dtor: holder.second
Trace dtor: holder.first
```

- 成员对象先于 `Holder` 自己的构造函数体构造，顺序是声明顺序 `first_`、`second_`。
- 析构时先执行 `Holder` 自己的析构函数体，再逆序析构成员：`second_`、`first_`。
- 所以“对象本身”和“成员”的边界是：**构造时成员先、自己后；析构时自己先、成员后**。

> **记忆点：** 构造从里到外（成员 → 自己），析构从外到里（自己 → 成员），两个方向都按声明顺序 / 逆序。

## 5. 初始化列表的顺序陷阱

```cpp
class Reorder {
public:
    Reorder(int v) : b_(v), a_(b_) {}
private:
    int a_;
    int b_;
};
```

严格警告编译：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion \
    -c warn_reorder.cpp -o /tmp/warn_reorder.o
```

实际警告：

```text
warn_reorder.cpp: In constructor ‘Reorder::Reorder(int)’:
warn_reorder.cpp:6:9: warning: ‘Reorder::b_’ will be initialized after [-Wreorder]
    6 |     int b_;
      |         ^~
warn_reorder.cpp:5:9: warning:   ‘int Reorder::a_’ [-Wreorder]
    5 |     int a_;
      |         ^~
warn_reorder.cpp:3:5: warning:   when initialized here [-Wreorder]
    3 |     Reorder(int v) : b_(v), a_(b_) {}
      |     ^~~~~~~
warn_reorder.cpp:3:32: warning: member ‘Reorder::b_’ is used uninitialized [-Wuninitialized]
    3 |     Reorder(int v) : b_(v), a_(b_) {}
      |                                ^~
```

虽然初始化列表写的是 `b_` 在前，实际初始化顺序仍然是声明顺序 `a_`、`b_`。于是 `a_(b_)` 先执行，`b_` 还没初始化，读到的是不确定值，属于 UB。这个程序只编译看警告，**不要运行**。

> **记忆点：** 初始化列表的书写顺序不决定实际顺序；实际顺序永远看成员声明。写反了轻则 `-Wreorder`，重则 UB。

## 6. 问题 1：`explicit` 的用法

### 6.1 实测：哪些写法会被阻止

`fail_explicit.cpp`：

```cpp
class Box {
public:
    explicit Box(int size);
    int size() const;
};

void take(Box b);

int main() {
    Box a(10);       // 正确：直接初始化
    Box b = 10;      // 错误：explicit 阻止隐式转换
    take(10);        // 错误：同样需要隐式转换
}
```

用 `-DEXPLICIT` 打开那两行，实际报错：

```text
fail_explicit.cpp: In function ‘int main()’:
fail_explicit.cpp:14:13: error: conversion from ‘int’ to non-scalar type ‘Box’ requested
   14 |     Box b = 10; // 错误：Box(int) 是 explicit 的，不能隐式转换
      |             ^~
fail_explicit.cpp:15:10: error: could not convert ‘10’ from ‘int’ to ‘Box’
   15 |     take(10); // 错误：Box(int) 是 explicit 的，不能隐式转换
```

把 `explicit` 去掉，用同样三行代码再编译一次（`g++ -std=c++20 -Wall -Wextra -Wpedantic -c`），编译通过、无错误。这说明阻止转换的就是 `explicit` 本身，不是别的原因。

`fail_explicit_variants.cpp` 再把各种写法放在一起对照：

```cpp
Box a(10);     // OK：直接初始化
Box b{10};     // OK：直接列表初始化
Box c = 10;    // 错误：复制初始化
Box d = {10};  // 错误：复制列表初始化
take(10);      // 错误：按值传参需要隐式转换
return 10;     // 错误：返回时隐式转换为 Box
```

实际报错共 4 处：

```text
fail_explicit_variants.cpp:17:12: error: could not convert ‘10’ from ‘int’ to ‘Box’
   17 |     return 10;  // 错误：返回时隐式转换为 Box
fail_explicit_variants.cpp:23:13: error: conversion from ‘int’ to non-scalar type ‘Box’ requested
   23 |     Box c = 10;   // 错误：复制初始化
fail_explicit_variants.cpp:24:16: error: converting to ‘Box’ from initializer list would use explicit constructor ‘Box::Box(int)’
   24 |     Box d = {10}; // 错误：复制列表初始化
fail_explicit_variants.cpp:25:10: error: could not convert ‘10’ from ‘int’ to ‘Box’
   25 |     take(10);     // 错误：按值传参需要隐式转换
```

### 6.2 `explicit` 到底阻止了什么

`explicit` 通常加在**转换构造函数**上（能被一个实参调用的构造函数：单参数，或多参数但其余都有默认值）。C++11 起也可以加在多参数构造函数上，用来阻止 `Box b = {1, 2};` 这种复制列表初始化。核心作用是阻止“编译器替你自动转换”的通道，也就是**复制初始化 / 隐式转换**。

| 写法 | 初始化类别 | `explicit Box(int)` 下 |
|---|---|---|
| `Box a(10);` | 直接初始化 | ✅ |
| `Box a{10};` | 直接列表初始化 | ✅ |
| `static_cast<Box>(10)` | 显式转换 | ✅ |
| `Box b = 10;` | 复制初始化 | ❌ |
| `Box b = {10};` | 复制列表初始化 | ❌ |
| `take(10);` | 按值传参的隐式转换 | ❌ |
| `return 10;` | 返回值的隐式转换 | ❌ |
| `Box b = Box(10);` | 先显式构造，再从 `Box` 复制初始化 | ✅ |
| `Box b = static_cast<Box>(10);` | 显式转换后复制初始化 | ✅ |

判断方法：看有没有“要求把 `int` 自动变成 `Box`”这一步。`Box(10)` 和 `Box{10}` 是你明确要求构造；`= 10`、`take(10)`、`return 10` 是编译器替你做转换。

### 6.3 为什么单参数构造函数通常默认加 `explicit`

不加 `explicit` 时，`Box` 会悄悄出现在任何需要 `Box` 的地方：

```cpp
void take(Box b);
take(10);          // 10 被悄悄变成 Box(10)
Box b = 10;        // 同上
```

这种“隐式转换”读代码时很难发现，容易把错误数值带进函数。单参数构造函数默认 `explicit` 是防御性写法；只有**确实希望隐式转换**时才去掉，例如 `std::string` 从字符串字面量构造是有意为之，所以它没有 `explicit`。

C++11 起 `explicit` 也可以修饰转换运算符，例如 `explicit operator bool()`：`if (obj)` 这种语境转换仍然可以，`bool b = obj;` 这种复制初始化会被阻止。这个主题今天不展开。

### 6.4 `Box b = Box(10);` 和 `Box b(10);` 的区别

两种写法最终都得到一个 `Box`，但语法路径不同：

- `Box b(10);`：直接初始化，直接调用 `Box(int)`。
- `Box b = Box(10);`：先显式构造一个 `Box` 临时对象（这里是 `Box(10)`，显式调用不受 `explicit` 限制），再用它复制初始化 `b`。C++17 的 guaranteed copy elision 会把这个复制省略掉，实际只构造一个对象。

`explicit` 阻止的是 `=` 右边“从 `int` 自动转成 `Box`”，不阻止 `Box(10)` 这个显式构造。所以 `Box b = Box(10);` 在 `explicit` 下仍然合法，实测通过。

> **记忆点：** `explicit` 只关“隐式/复制初始化”这扇门，不关“直接构造、`T{...}`、`static_cast`”这几扇门；看到 `=` 和按值传参就要先想它。

## 7. 问题 2：构造函数末尾的分号什么时候有、什么时候没有

### 7.1 三种形式

```cpp
class Point {
public:
    Point();                                  // ① 声明：必须有分号
    Point(int x, int y) : x_(x), y_(y) {}     // ② 类内定义：不要分号
    Point(const Point&) = delete;             // ③ = delete：必须有分号
    Point& operator=(const Point&) = default; // ④ = default：必须有分号
private:
    int x_;                                   // ⑤ 数据成员：必须有分号
};                                            // ⑥ 类定义结尾：必须有分号
```

类外定义同理：

```cpp
Point::Point() : x_(0), y_(0) {}   // 定义：不要分号
```

### 7.2 实测

**多余的分号合法。** `ctors.cpp` 里就有一处：

```cpp
Point(int x, int y) : x_(x), y_(y) {};   // 行尾多了一个 ;
```

用 `-std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror` 编译整个 `ctors.cpp`，**通过、零警告**。在函数体 `{}` 后面多写的 `;` 是一个空声明（empty declaration），语法合法，`-Wpedantic` 也不报。

**漏写 `= default` 的分号是错误。** `fail_missing_semicolon.cpp`：

```cpp
class S {
public:
    S() = default
};
```

实际报错：

```text
fail_missing_semicolon.cpp:4:11: error: expected ‘;’ at end of member declaration
    4 |     S() = default
      |           ^~~~~~~
      |                  ;
```

### 7.3 规则小结

| 形式 | 末尾分号 | 说明 |
|---|---|---|
| `Point();` | 必须有 | 只有声明，没有函数体 |
| `Point() {}`（类内） | 不要 | 有函数体；多写 `;` 合法但多余 |
| `Point::Point() {}`（类外） | 不要 | 同上 |
| `Point() = default;` | 必须有 | 没有函数体，靠分号结束声明 |
| `Point() = delete;` | 必须有 | 同上 |
| `int x_;` | 必须有 | 数据成员声明 |
| `class Point { ... };` | 必须有 | 类定义结尾 |

口诀：**对函数定义而言，有 `{}` 就不用分号，没 `{}` 就要分号**。`= default` / `= delete` 虽然没有函数体，但它们是声明，所以必须有分号。注意类定义 `class Point { ... };` 的 `}` 后面仍然要分号，因为类定义本身是一条声明。

> **记忆点：** 分号跟着“声明”走，不跟着“定义”走；`{}` 已经结束了定义，`= default` / `= delete` 还没有。

## 8. 问题 3：`Point();` 和 `NonCopyable() = default;` 会初始化什么

先把结论说清楚：**这两个写法本身都不初始化任何数据成员。** 前者只是声明，后者是让编译器生成定义；到底初始化什么，要看构造函数体 / 初始化列表 / 默认成员初始化器。

### 8.1 `Point();` 是声明，不是定义

```cpp
class Point {
public:
    Point();   // 声明：告诉编译器“这个类有个默认构造，定义在后面”
    int x_;
    int y_;
};
```

- 在类里写 `Point();` 不会生成任何代码，也不会给 `x_`、`y_` 赋值。
- 它让 `Point` 成为“有用户声明的默认构造函数”的类，编译器不会再隐式生成一个默认构造。
- 必须再提供定义：

```cpp
Point::Point() : x_(0), y_(0) {}   // 这里才决定初始化什么
```

本仓库的 `ctors.cpp` 就是这种写法，所以 `Point origin;` 得到 `(0, 0)`。如果定义改成 `Point::Point() {}`，`x_`、`y_` 就是不确定值。

如果只声明、不定义，又使用了它，编译能过，链接失败。实际链接错误：

```text
/usr/bin/ld: /tmp/...o: in function `main':
fail_ctor_undefined.cpp:(.text+0x23): undefined reference to `X::X()'
collect2: error: ld returned 1 exit status
```

### 8.2 `NonCopyable() = default;` 是定义

```cpp
class NonCopyable {
public:
    NonCopyable() = default;   // 定义：让编译器生成默认构造
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};
```

- `= default` 写在类内首次声明处，表示“请编译器生成这个函数的定义”，并且是 `inline` 的。
- 它生成的是**默认构造**：对类类型成员调用其默认构造，对标量成员做**默认初始化**（值不确定），对写了默认成员初始化器的成员用那个值。它**不会**把 `int` 清零。
- `NonCopyable` 没有数据成员，所以这个默认构造实际什么都不用做；它的作用是“把这个默认构造补回来”（见 8.5）。
- 在首次声明处 `= default` 还能保持“trivial（平凡）”属性；如果先声明 `T();`、再在类外写 `T::T() = default;`，它就变成 user-provided，不再是 trivial。

### 8.3 实测：默认初始化 vs 值初始化

`default_init.cpp` 用 `-ftrivial-auto-var-init=pattern` 编译，让没被初始化的自动变量填成 `0xFE` 模式（打印为 `-16843010`），这样能直接看出“没初始化”，而不是读到随机值：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -ftrivial-auto-var-init=pattern default_init.cpp -o /tmp/d04_default_init
/tmp/d04_default_init
```

实际输出：

```text
== 默认初始化：T x; ==
Declared      : a=-16843010, b=-16843010
Defaulted     : a=-16843010, b=-16843010
WithInitList  : a=0, b=0
WithMemberInit: a=0, b=0

== 值初始化：T x{}; ==
Declared{}         : a=-16843010, b=-16843010
Defaulted{}        : a=0, b=0
OutOfClassDefault{}: a=-16843010, b=-16843010
```

逐项解释：

| 类 | 默认构造 | `T x;` | `T x{};` |
|---|---|---|---|
| `Declared` | `Declared();` + 类外 `{}`（user-provided） | 不确定值 | 调用用户构造，仍不确定值 |
| `Defaulted` | 首次声明处 `= default`（非 user-provided） | 不确定值 | **先零初始化**，得到 0 |
| `OutOfClassDefault` | 先声明，类外 `= default`（user-provided） | 不确定值 | 调用用户构造，仍不确定值 |
| `WithInitList` | 初始化列表 `: a(0), b(0)` | 0 | 0 |
| `WithMemberInit` | `= default` + `int a = 0;` | 0 | 0 |

这里最反直觉的是 `Defaulted{}` 和 `OutOfClassDefault{}` 的差别：两者都用了 `= default`，但只有**首次声明处** `= default` 的那个保持“非 user-provided”，值初始化 `T{}` 才会先零初始化。`Declared{}` 虽然值初始化，但默认构造是用户提供的，所以不会补零。

### 8.4 `Point();` 与 `NonCopyable() = default;` 对照

| | `Point();` | `NonCopyable() = default;` |
|---|---|---|
| 性质 | 声明 | 定义（首次声明处 default） |
| 是否生成代码 | 不生成，等类外定义 | 编译器生成 |
| 不写定义会怎样 | 使用时报链接错误 | 不存在这个问题 |
| 初始化标量成员 | 不初始化；取决于后续定义 | 默认初始化，不确定值 |
| 是否保持 trivial | 非 trivial（首个声明没写 `= default`，定义后算 user-provided） | 首次声明处 default 时保持 trivial |
| 对空类 | 构造一个空对象 | 构造一个空对象 |

所以“`Point();` 会初始化什么”这个问题本身要拆开：

- 只看这一行：**什么都不初始化**，它只是声明。
- 看本仓库的完整定义 `Point::Point() : x_(0), y_(0) {}`：初始化 `x_`、`y_` 为 0。
- 如果定义是 `Point::Point() {}`：`x_`、`y_` 不确定。
- 如果 `Point` 写成 `Point() = default;` 且没有默认成员初始化器：`x_`、`y_` 不确定；但 `Point p{};` 会先零初始化，得到 0（前提是 default 写在首次声明处）。

> **记忆点：** `T();` 是“声明有这么一个构造”，`T() = default;` 是“让编译器现在生成它”；两者都不等于“把成员清零”，清零靠初始化列表或默认成员初始化器。

### 8.5 为什么 `NonCopyable` 还要写 `NonCopyable() = default;`

因为**声明任何构造函数都会抑制隐式默认构造**，`= delete` 的拷贝构造也算“用户声明的构造函数”。如果删掉 `NonCopyable() = default;`：

```cpp
class NonCopyable {
public:
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};

int main() {
    NonCopyable a;  // 错误
}
```

实际报错：

```text
fail_noncopyable_no_default.cpp: In function ‘int main()’:
fail_noncopyable_no_default.cpp:9:17: error: no matching function for call to ‘NonCopyable::NonCopyable()’
    9 |     NonCopyable a;  // 错误：没有可用的默认构造函数
      |                 ^
fail_noncopyable_no_default.cpp:4:5: note: candidate: ‘NonCopyable::NonCopyable(const NonCopyable&)’ (deleted)
    4 |     NonCopyable(const NonCopyable&) = delete;
      |     ^~~~~~~~~~~
fail_noncopyable_no_default.cpp:4:5: note:   candidate expects 1 argument, 0 provided
```

补上 `NonCopyable() = default;` 后，`NonCopyable nc;` 才能正常构造（实测 `ctors.cpp` 输出 `NonCopyable constructed`）。

> **记忆点：** 只要类里声明了任何构造函数（包括 `= delete` 的拷贝构造），隐式默认构造就没了；需要默认构造就补 `T() = default;`。

## 9. `= default` 与 `= delete`

### 9.1 `= default`

- 让编译器生成特殊成员函数的定义，可以写在类内首次声明处，也可以写在类外。
- 类内首次声明处 `= default`：保持 trivial / inline。
- 类外再 `= default`：算 user-provided，可能失去 trivial 属性（8.3 已实测）。
- 常用于“我声明了这个特殊成员，但不想手写实现”。

### 9.2 `= delete`

- 函数仍然被声明，参与重载解析，但任何调用都会在**编译期**报错。
- 不仅能用于特殊成员，也能用于普通函数 / 重载（例如禁用某个重载）。
- 用来禁止拷贝，比“声明成 private 且不定义”的 C++03 写法错误信息更清楚；比“写一个空实现”安全得多。

`fail_noncopyable.cpp` 的实际报错：

```text
fail_noncopyable.cpp: In function ‘int main()’:
fail_noncopyable.cpp:11:21: error: use of deleted function ‘NonCopyable::NonCopyable(const NonCopyable&)’
   11 |     NonCopyable b = a;  // 错误：拷贝构造被 delete
      |                     ^
fail_noncopyable.cpp:5:5: note: declared here
    5 |     NonCopyable(const NonCopyable&) = delete;
fail_noncopyable.cpp:13:9: error: use of deleted function ‘NonCopyable& NonCopyable::operator=(const NonCopyable&)’
   13 |     c = a;              // 错误：拷贝赋值被 delete
      |         ^
fail_noncopyable.cpp:6:18: note: declared here
    6 |     NonCopyable& operator=(const NonCopyable&) = delete;
```

如果写成“空实现的拷贝构造”，编译能过，但两个对象会共享同一份资源，析构时 double-free / double-close。`= delete` 把问题提前到编译期。

> **记忆点：** `= default` 是“生成”，`= delete` 是“禁止”；delete 在编译期拦截，空实现会把灾难留到运行期。

## 10. 易错点

1. **以为 `int` 成员自动为 0**：不会。必须写初始化列表、默认成员初始化器，或在“非 user-provided 默认构造 + 值初始化”下才会被零初始化。
2. **以为 `= default` 等于清零**：`= default` 生成的默认构造对 `int` 是默认初始化，值不确定。
3. **把 `Point();` 当成定义**：它只是声明；不定义就使用会链接失败。
4. **把初始化列表的书写顺序当成实际顺序**：实际顺序永远看成员声明顺序。
5. **忘记成员析构是逆序**：构造 `first_`、`second_`，析构先 `second_` 再 `first_`。
6. **在构造函数体里给 `const` 成员赋值**：报 `assignment of read-only member`；必须在初始化列表里初始化。
7. **在构造函数体里给引用成员“赋值”**：报 `uninitialized reference member`；即使已绑定，赋值也只是写穿，不是改绑。
8. **在 `{}` 后面多写分号还以为是错误**：合法，但没必要；真正要小心的是漏掉 `= default` / `= delete` 后面的分号。
9. **以为 `explicit` 会阻止 `Box(10)`**：不会。它只阻止复制初始化和隐式转换。
10. **以为 `Box b = Box(10);` 里 `explicit` 会阻止**：不会，`Box(10)` 是显式构造，`=` 只是从 `Box` 复制初始化。
11. **删掉拷贝构造后忘记补默认构造**：`= delete` 的拷贝构造也是用户声明的构造函数，会抑制隐式默认构造。
12. **把 `T x{}` 当成万能清零**：如果默认构造是 user-provided（包括类外 `= default`、`{}` 定义），值初始化也不会清零。

## 11. 待复习

- 初始化列表和函数体赋值，对 `const`、引用、无默认构造的类成员分别意味着什么？
- 成员的初始化顺序由什么决定？初始化列表写反会怎样？
- 局部对象、成员对象、对象自身的构造 / 析构边界分别在哪里？
- `explicit Box(int)` 分别阻止和允许哪些写法？`Box b = Box(10);` 为什么可以？
- 为什么单参数构造函数通常默认加 `explicit`？哪些标准库类型故意不加？
- 构造函数声明、类内定义、类外定义、`= default`、`= delete` 各自要不要分号？
- `Point();` 和 `Point() = default;` 在“是否生成代码、是否保持 trivial、是否初始化成员”上有什么区别？
- `T x;` 和 `T x{};` 在默认构造是否 user-provided 时分别初始化成什么？
- 为什么 `NonCopyable` 写了 `= delete` 的拷贝构造后，还要写 `NonCopyable() = default;`？
- `= delete` 和“空实现的拷贝构造”在出错时机上有什么区别？
