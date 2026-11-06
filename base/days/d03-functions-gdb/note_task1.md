# D03 任务 1：重载解析与默认参数

> 环境：WSL2 Ubuntu 24.04，g++ 13.3.0，CMake 3.28.3，Ninja。
> 代码目录：[code/](code/)
> 构建目录：`~/build/base`
> targets：`d03-funcs`（`overload.cpp`）、`d03-defaults`（`default_args.cpp`）。
>
> 本笔记记录的是实际编译、运行和失败实验的结果，不是凭记忆写的“应该怎样”。

## 0. 速查

- 重载解析优先级：**精确匹配 > 提升（promotion）> 转换（conversion）**。
- 整型提升：`char` / `signed char` / `unsigned char` / `short` / `bool` → `int`（`int` 装得下时）。
- 浮点提升：`float` → `double`。
- 精确匹配优先于任何提升；提升优先于任何转换。
- 同一等级有多个候选且没有更优者 → **歧义（ambiguous）**，编译失败。
- 默认参数：从右往左、只在声明写一次、调用点补全；和重载组合可能产生歧义。
- 返回值类型不参与重载；顶层 `const` 对按值参数被忽略。

## 1. 代码结构

| 文件 | 内容 | 是否进 CMake |
|---|---|---|
| `overload.cpp` | `pick` / `touch` / `ptr` 三组重载 | 是，target `d03-funcs` |
| `default_args.cpp` | `log` 的默认参数 | 是，target `d03-defaults` |
| `fail_overload_ambiguous.cpp` | `rank` 和 `h` 的歧义 | 否，故意失败 |
| `fail_default_order.cpp` | 默认参数不从右往左 | 否，故意失败 |
| `fail_default_redef.cpp` | 声明和定义重复写默认值 | 否，故意失败 |

CMake 里的新增 target：

```cmake
add_executable(d03-funcs days/d03-functions-gdb/code/overload.cpp)
target_compile_features(d03-funcs PRIVATE cxx_std_20)
target_compile_options(d03-funcs PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror)

add_executable(d03-defaults days/d03-functions-gdb/code/default_args.cpp)
target_compile_features(d03-defaults PRIVATE cxx_std_20)
target_compile_options(d03-defaults PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror)
```

## 2. 构建与运行

构建：

```bash
cmake -S /mnt/d/Project/CPP/CPPLearn/base -B ~/build/base
cmake --build ~/build/base --target d03-funcs d03-defaults -j"$(nproc)"
```

实际输出：

```text
[1/4] Building CXX object CMakeFiles/d03-defaults.dir/days/d03-functions-gdb/code/default_args.cpp.o
[2/4] Building CXX object CMakeFiles/d03-funcs.dir/days/d03-functions-gdb/code/overload.cpp.o
[3/4] Linking CXX executable d03-defaults
[4/4] Linking CXX executable d03-funcs
```

没有 `warning:` 行；两个 target 都带 `-Werror`，有警告会直接失败。两个程序退出码都是 0。

## 3. `pick`：精确匹配、提升、转换

### 3.1 代码

```cpp
void pick(int a) {
    std::cout << "pick(int): " << a << '\n';
}

void pick(double a) {
    std::cout << "pick(double): " << a << '\n';
}

void pick(float a) {
    std::cout << "pick(float): " << a << '\n';
}

int main() {
    pick(1);
    pick('a');
    pick(1.0f);
    pick(1.0);
}
```

### 3.2 实际输出

```text
== pick ==
pick(int): 1
pick(int): 97
pick(float): 1
pick(double): 1
```

### 3.3 逐行分析

| 调用 | 实参类型 | 候选与转换 | 选中的重载 | 理由 |
|---|---|---|---|---|
| `pick(1)` | `int` | `int` 精确；`int→float`、`int→double` 是 conversion | `pick(int)` | 精确匹配 |
| `pick('a')` | `char` | `char→int` 是 promotion；`char→float`、`char→double` 是 conversion | `pick(int)` | 提升优于转换 |
| `pick(1.0f)` | `float` | `float` 精确；`float→double` 是 promotion；`float→int` 是 conversion | `pick(float)` | 精确匹配 |
| `pick(1.0)` | `double` | `double` 精确；`double→float`、`double→int` 是 conversion | `pick(double)` | 精确匹配 |

`pick('a')` 打印 `97`，因为 `'a'` 的 ASCII 值是 97；它被提升成 `int` 后，`std::cout << int` 按整数输出，不是字符 `a`。

### 3.4 如果删掉 `pick(float)`

`pick(1.0f)` 的选择会改变：

| 候选 | 转换 | 等级 |
|---|---|---|
| `pick(double)` | `float → double` 浮点提升 | promotion |
| `pick(int)` | `float → int` 浮点—整型转换 | conversion |

此时 `pick(double)` 会通过提升胜出。也就是说，**新增一个重载可能改变已有调用的选择**，这是重载集合的脆弱点之一。

## 4. `touch`：`int&` 与 `const int&`

### 4.1 代码

```cpp
void touch(int& x) {
    std::cout << "touch(int&): " << x << '\n';
}

void touch(const int& x) {
    std::cout << "touch(const int&): " << x << '\n';
}

int x = 1;
const int cx = 2;
touch(x);
touch(cx);
touch(3);
```

### 4.2 实际输出

```text
== touch ==
touch(int&): 1
touch(const int&): 2
touch(const int&): 3
```

### 4.3 为什么

- `touch(x)`：`x` 是非 const 左值，`int&` 是精确匹配，`const int&` 需要限定转换；选 `int&`。
- `touch(cx)`：`cx` 是 const 左值，不能绑定到 `int&`（否则可以通过引用改 const 对象）；只能选 `const int&`。
- `touch(3)`：`3` 是右值，非 const 左值引用不能绑定右值；`const int&` 可以绑定，选它。

`touch(3)` 的临时对象生命周期会延长到整个函数调用表达式结束；函数内部看到的引用是有效的。

## 5. `ptr`：`int*` 与 `std::nullptr_t`

### 5.1 代码

```cpp
#include <cstddef>

void ptr(int* p) {
    std::cout << "ptr(int*): " << static_cast<const void*>(p) << '\n';
}

void ptr(std::nullptr_t) {
    std::cout << "ptr(nullptr_t)\n";
}

ptr(nullptr);

int* p = nullptr;
ptr(p);
```

### 5.2 实际输出

```text
== ptr ==
ptr(nullptr_t)
ptr(int*): 0
```

### 5.3 为什么

- `nullptr` 的类型是 `std::nullptr_t`，和 `ptr(std::nullptr_t)` 精确匹配；转换成 `int*` 只是 conversion，排在后面。
- `p` 的类型是 `int*`，和 `ptr(int*)` 精确匹配。
- `ptr(int*)` 里把空指针转成 `const void*` 打印，得到 `0`。

不要把 `NULL` 和 `nullptr` 混为一谈：`NULL` 通常是整型常量 `0`，重载选择可能和 `nullptr` 不同。C++ 里优先用 `nullptr`。

## 6. 默认参数

### 6.1 代码

```cpp
// 默认值只写在声明里；定义不能重复写。
void log(const char* msg, int level = 0, bool flush = false);

void log(const char* msg, int level, bool flush) {
    std::cout << "log: " << msg
              << ", level=" << level
              << ", flush=" << (flush ? "true" : "false")
              << '\n';
}

int main() {
    log("hello");
    log("hello", 1);
    log("hello", 1, true);
}
```

### 6.2 实际输出

```text
log: hello, level=0, flush=false
log: hello, level=1, flush=false
log: hello, level=1, flush=true
```

### 6.3 规则

1. **从右往左**：某个参数有默认值，它右边的参数都必须有默认值。
2. **声明和定义只能写一处**：通常在声明（头文件）写，定义不写。
3. **默认参数不是函数类型的一部分**：不能靠默认参数区分重载。
4. **调用点补全**：编译器在调用处把缺省的实参填进去；默认值表达式在调用时求值。
5. **和重载组合可能歧义**：`void h(int); void h(int, int = 0);` 调用 `h(1)` 时两个候选都能匹配。

## 7. 失败实验（故意编译失败）

复现命令（在 `code/` 下执行，三个文件分别替换文件名）：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion \
    -c fail_overload_ambiguous.cpp -o /tmp/fail_ambiguous.o
```

### 7.1 重载歧义：`rank` 和 `h`

`fail_overload_ambiguous.cpp`：

```cpp
void rank(long);
void rank(double);

void h(int);
void h(int, int = 0);

int main() {
    rank(1);  // int -> long 和 int -> double 都是 conversion
    h(1);     // h(int) 与 h(int, int = 0) 都能调用
    return 0;
}
```

实际报错：

```text
fail_overload_ambiguous.cpp: In function ‘int main()’:
fail_overload_ambiguous.cpp:11:9: error: call of overloaded ‘rank(int)’ is ambiguous
   11 |     rank(1);  // int -> long 和 int -> double 都是 conversion
      |     ~~~~^~~
fail_overload_ambiguous.cpp:4:6: note: candidate: ‘void rank(long int)’
    4 | void rank(long);
      |      ^~~~
fail_overload_ambiguous.cpp:5:6: note: candidate: ‘void rank(double)’
    5 | void rank(double);
      |      ^~~~
fail_overload_ambiguous.cpp:12:6: error: call of overloaded ‘h(int)’ is ambiguous
   12 |     h(1);     // h(int) 与 h(int, int = 0) 都能调用
      |     ~^~~
fail_overload_ambiguous.cpp:7:6: note: candidate: ‘void h(int)’
    7 | void h(int);
      |      ^
fail_overload_ambiguous.cpp:8:6: note: candidate: ‘void h(int, int)’
    8 | void h(int, int = 0);
      |      ^
```

两条错误的原因不同：

- `rank(1)`：`int → long` 和 `int → double` 都是 conversion rank，没有更优者。
- `h(1)`：`h(int)` 是精确匹配；`h(int, int = 0)` 用默认值补第二个参数后，第一个参数也是精确匹配。两个候选的转换等级相同，默认参数不提供“更好”的排序，所以歧义。

### 7.2 默认参数不从右往左

`fail_default_order.cpp`：

```cpp
void bad(int a = 1, int b);
```

实际报错：

```text
fail_default_order.cpp:4:25: error: default argument missing for parameter 2 of ‘void bad(int, int)’
    4 | void bad(int a = 1, int b);
      |                     ~~~~^
fail_default_order.cpp:4:14: note: ...following parameter 1 which has a default argument
    4 | void bad(int a = 1, int b);
      |          ~~~~^~~~~
```

参数 1 有默认值，但右边的参数 2 没有；调用 `bad(?)` 时无法确定缺省的是哪个，所以禁止。

### 7.3 声明和定义重复写默认值

`fail_default_redef.cpp`：

```cpp
void f(int x = 1);

void f(int x = 1) {
}
```

实际报错：

```text
fail_default_redef.cpp:6:6: error: default argument given for parameter 1 of ‘void f(int)’ [-fpermissive]
    6 | void f(int x = 1) {
      |      ^
fail_default_redef.cpp:4:6: note: previous specification in ‘void f(int)’ here
    4 | void f(int x = 1);
      |      ^
fail_default_redef.cpp: In function ‘void f(int)’:
fail_default_redef.cpp:6:12: warning: unused parameter ‘x’ [-Wunused-parameter]
    6 | void f(int x = 1) {
      |        ~~~~^~~~~
```

同一个参数只能在一处指定默认值。`[-fpermissive]` 表示 GCC 把这个错误归为可以通过 `-fpermissive` 降级为警告的类别；不要用 `-fpermissive` 掩盖它。

## 8. 重载解析规则总结

### 8.1 标准转换序列的等级

从好到差：

1. **精确匹配（Exact Match）**
   - 类型完全相同；
   - 数组退化为指针、函数退化为函数指针；
   - 左值到右值转换；
   - 限定转换（例如 `int*` → `const int*`）。
2. **提升（Promotion）**
   - 整型提升：`char`、`short`、`bool` 等 → `int`；
   - 浮点提升：`float` → `double`。
3. **转换（Conversion）**
   - 整型之间转换：`int` → `long`、`int` → `unsigned`；
   - 浮点—整型转换：`double` → `int`、`int` → `float`；
   - 指针转换、布尔转换等。

比较时，一个候选如果所有参数都不比另一个差，且至少一个更好，就更优；否则可能歧义。

### 8.2 重载的签名边界

- **返回值类型不参与重载**：`int f();` 和 `double f();` 不能共存。
- **顶层 `const` 对按值参数被忽略**：`void f(int)` 和 `void f(const int)` 是同一个签名。
- **指针/引用的底层 `const` 不同**：`void f(int*)` 和 `void f(const int*)` 是不同重载。
- **默认参数不是签名的一部分**：`void h(int)` 和 `void h(int, int = 0)` 可以共存，但 `h(1)` 会歧义。

## 9. 易错点

1. **把 `1.0f` 当成 `double`**：`1.0f` 是 `float`，`1.0` 是 `double`；有没有 `pick(float)` 会改变选择。
2. **以为 `char` 会选 `pick(char)`**：没有 `pick(char)` 时，`char` 通过整型提升选 `pick(int)`，打印 ASCII 值 97。
3. **忽略新增重载的副作用**：加 `pick(float)` 后，`pick(1.0f)` 从“提升到 double”变成“精确匹配 float”。
4. **把歧义当运行时问题**：歧义是编译期错误，程序根本不会生成。
5. **给 `int&` 绑定右值**：`touch(3)` 只能匹配 `const int&`；非 const 左值引用不能绑定临时对象。
6. **用 `NULL` 代替 `nullptr`**：`NULL` 常是整型 0，重载选择可能不同；`nullptr` 有独立的 `std::nullptr_t` 类型。
7. **默认参数写在定义里**：声明和定义只能一处写默认值；否则报 `default argument given ...`。
8. **默认参数从左边开始省略**：默认参数必须从右往左；`void bad(int a = 1, int b);` 直接编译失败。
9. **只看输出不验证选择**：输出 `int: 97` 要结合候选集和转换等级判断，不能只凭“看起来像”。
10. **用 `(int)` 硬压强转**：需要指定类型时优先用 `static_cast`，并说明为什么需要这次转换。

## 10. 待复习

- 精确匹配、提升、转换三档，各举一个例子。
- `char → int` 为什么是提升，`char → double` 为什么是转换？
- `float → double` 是提升，`float → int` 是转换；`pick(1.0f)` 在有无 `pick(float)` 时分别选谁？
- `int&` / `const int&` 对左值、const 左值、右值的绑定规则。
- `nullptr`、`NULL`、`0` 在重载里的区别。
- 默认参数的三条规则：从右往左、只写一次、不是函数类型的一部分。
- 为什么 `h(int)` / `h(int, int = 0)` 调用 `h(1)` 会歧义？
- 重载歧义在编译期还是运行期发生？
- 返回值类型、顶层 `const`、默认参数分别是否参与重载签名？
