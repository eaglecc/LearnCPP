# D02 任务 2：const / constexpr / auto

> 对应 `types.cpp` 第 7–9 节。普通构建必须零警告：
>
> ```bash
> g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror types.cpp -o types
> ./types
> ```

## 今天解决的问题

- `const` 与指针的三种组合：`const int*`、`int* const`、`const int* const`，能说清各自能改什么、不能改什么。
- `constexpr`：编译期求值、`static_assert`、`constexpr` 函数也能运行时调用；`const` 不等于 `constexpr`。
- `auto` 的推导规则：顶层 `const`、引用、数组退化分别会怎样；用 `static_assert` 验证真实类型。

## 核心概念 & 为什么这样设计

### 1. `const` 与指针

#### 从右往左读

| 声明 | 读法 | 能否改 `*p` | 能否改 `p` 的指向 |
|---|---|---|---|
| `const int* p1` | `p1` 是「指向 const int 的指针」 | 否 | **是** |
| `int* const p2` | `p2` 是「const 指针，指向 int」 | **是** | 否 |
| `const int* const p3` | `p3` 是「const 指针，指向 const int」 | 否 | 否 |

- `const int` 和 `int const` 完全等价；`const` 默认修饰它**左边**的类型，左边没有类型时才修饰右边。
- `const int* p` 修饰的是 **`int`**，不是 `p`：`p` 本身可以改指向。
- `int* const p` 修饰的是 **`p`**：指针本身不能改指向，但它指向的数据可以改。
- `int* const p2` 是 **const 对象**，必须在声明时初始化；`const int* p1` 可以先不初始化（但很危险）。

#### 代码与编译结果

```cpp
int value = 1;
int other = 10;

const int* p1 = &value;        // 指向 const int
int* const p2 = &value;        // const 指针，指向 int
const int* const p3 = &value;  // 两者都 const

p1 = &other;   // OK：改 p1 的指向
*p2 = 2;       // OK：通过 p2 改 value（value 本身不是 const）

// *p1 = 3;      // 错误：*p1 是 const int
// p2 = &other;  // 错误：p2 是 const 指针
// *p3 = 3;      // 错误：*p3 是 const int
// p3 = &other;  // 错误：p3 是 const 指针
```

g++ 13.3.0 的实际报错（把四行取消注释后）：

```text
const_fail.cpp:8:9:  error: assignment of read-only location '* p1'
const_fail.cpp:9:8:  error: assignment of read-only variable 'p2'
const_fail.cpp:10:9: error: assignment of read-only location '*(const int*)p3'
const_fail.cpp:11:8: error: assignment of read-only variable 'p3'
```

`types.cpp` 的运行结果：

```text
== 7. const 与指针 ==
value = 2, other = 10, *p1 = 10, *p3 = 2
```

- `*p2 = 2` 把 `value` 改成了 2；
- `p1` 改指向 `other`，所以 `*p1 == 10`；
- `p3` 仍指向 `value`，所以 `*p3 == 2`。

#### 顶层 const 与底层 const

- **顶层 const（top-level const）**：对象本身是 const，例如 `int* const p2`。
- **底层 const（low-level const）**：指针/引用指向的对象是 const，例如 `const int* p1`。
- 拷贝一个指针时，顶层 const 会被丢掉，底层 const 会保留：

```cpp
const int* pc = &value;
auto q = pc;           // const int*：底层 const 保留

int* const cp = &value;
auto q2 = cp;          // int*：顶层 const 被丢掉
```

这个区别在下面 `auto` 部分还会用到。

> 注意：`const` 只表示“不能通过这个名字修改”，**不表示编译期常量**。`const int x = f();` 是合法的，但 `x` 不是 `constexpr`。

### 2. `constexpr`

#### 基本用法

```cpp
constexpr int square(int x) { return x * x; }

static_assert(square(5) == 25);  // 编译期求值
constexpr int ct = square(6);    // 编译期求值
int rt = square(7);              // constexpr 函数也能在运行时调用
```

- `constexpr` 函数在参数是常量表达式、上下文要求常量时，会在**编译期**求值。
- 它也可以像普通函数一样在运行时调用（`square(7)`），不会强制每次都在编译期算。
- `constexpr` 变量隐含 `const`，并且必须用常量表达式初始化。
- `constexpr` 函数隐含 `inline`，可以安全地定义在头文件里。

#### `const` 不等于 `constexpr`

```cpp
int with_cout(int x) {
    std::cout << x;
    return x * x;
}

const int runtime = with_cout(4);  // const，但不是 constexpr
constexpr int bad = runtime;       // 错误
```

g++ 13.3.0 的报错：

```text
error: the value of 'runtime' is not usable in a constant expression
note: 'runtime' was not initialized with a constant expression
```

- `const` 只保证运行期不能通过这个名字改值；
- `constexpr` 要求**编译期**就能求出值。
- 只有 `const` 且用常量表达式初始化的整型变量，才可能被当作常量表达式使用；`const int x = f();` 不行。

#### 不能是 `constexpr` 的函数

含 `std::cout` 的函数不能用于编译期求值：

```cpp
constexpr int with_cout(int x) {   // 声明成 constexpr
    std::cout << x;                // 但这里调用了非 constexpr 的 operator<<
    return x * x;
}

int main() {
    constexpr int y = with_cout(3);  // 错误：无法在常量表达式中调用
}
```

g++ 13.3.0 的实际输出：

```text
warning: call to non-'constexpr' function
         'std::basic_ostream<...>::operator<<(int)' [-Winvalid-constexpr]

error: 'constexpr int with_cout(int)' called in a constant expression
```

- GCC 对函数定义先给 `-Winvalid-constexpr` 警告，真正在常量表达式里调用时再报 error。
- 常见的“不能 constexpr”原因：I/O、调用非 constexpr 函数、`reinterpret_cast`、`asm`、`goto`、未初始化变量等。
- C++20 放宽了一些限制（例如允许 `new`/`delete` 在常量求值中使用），但 I/O 仍然只能运行时做。
- 延伸：C++20 的 `consteval` 要求**必须**在编译期求值，D7 再展开；`constexpr` 是“可以编译期，也可以运行期”。

`types.cpp` 的运行结果：

```text
== 8. constexpr ==
with_cout(4) called
with_cout(3) called
square(6) = 36, square(7) = 49
with_cout(3) = 9, runtime_const = 16
```

### 3. `auto`

#### 推导规则速记

`auto` 按**值**推导，规则类似模板的按值形参：

- 丢掉**顶层 const/volatile**；
- 丢掉**引用**；
- 数组/函数退化为指针。

`auto&` 推导引用，因此会保留 `const`，也能保留数组类型。

| 写法 | 推导结果 | 原因 |
|---|---|---|
| `const int ci = 1; auto a = ci;` | `int` | 顶层 const 被丢掉 |
| `auto& r = ci;` | `const int&` | 引用绑定到 const 对象，const 保留 |
| `auto r2 = ci;` | `int` | 按值推导，const 丢掉 |
| `constexpr int cci = 42; auto y = cci;` | `int` | constexpr 的 const 也被丢掉 |
| `int arr[5]; auto p = arr;` | `int*` | 数组退化为指针 |
| `auto& arr_ref = arr;` | `int(&)[5]` | 引用保留数组类型 |

#### 代码与 `static_assert` 验证

```cpp
const int ci = 1;
constexpr int cci = 42;

auto a = ci;    // int
auto& r = ci;   // const int&
auto r2 = ci;   // int
auto y = cci;   // int

static_assert(std::is_same_v<decltype(a), int>);
static_assert(std::is_same_v<decltype(r), const int&>);
static_assert(std::is_same_v<decltype(r2), int>);
static_assert(std::is_same_v<decltype(y), int>);

a = 42;         // OK：a 是 int，不是 const int
// r = 2;       // 错误：r 是 const int&
```

`r = 2;` 的实际报错：

```text
error: assignment of read-only reference 'r'
```

数组部分：

```cpp
int arr[5] = {1, 2, 3, 4, 5};
auto p = arr;         // int*
auto& arr_ref = arr;  // int(&)[5]

static_assert(std::is_same_v<decltype(p), int*>);
static_assert(std::is_same_v<decltype(arr_ref), int(&)[5]>);
static_assert(sizeof(arr_ref) == sizeof(arr));  // 20 == 20
```

- `auto p = arr` 发生数组退化，`p` 是 `int*`，`sizeof(p) == 8`；
- `auto& arr_ref = arr` 保留数组类型，`sizeof(arr_ref) == 20`。
- 这就是任务 3 里“数组传参后 `sizeof` 变小”的前置知识。

#### `typeid` 的坑

用 `typeid(...).name()` 也能看类型，但它在 GCC 上输出的是修饰名，而且**会剥掉引用和顶层 const**：

```text
typeid(a).name()       = i
typeid(r).name()       = i
typeid(p).name()       = Pi
typeid(arr_ref).name() = A5_i
```

- `r` 明明是 `const int&`，`typeid` 只给出 `i`（int）；
- `A5_i` 表示“5 个 int 的数组”；
- 所以验证精确类型用 `static_assert(std::is_same_v<decltype(x), T>)` 更可靠，`typeid` 只适合粗略观察。

#### `auto` 与 `decltype` 的区别

- `decltype(ci)` 得到**声明时的类型** `const int`；
- `auto a = ci;` 得到 `int`（丢掉 const）。
- `decltype((ci))` 带一层括号，得到 `const int&`：括号里是左值表达式，`decltype` 对左值表达式产生引用类型。
- 想保留 const 或引用，显式写 `const auto` / `auto&` / `decltype(auto)`。

`types.cpp` 的运行结果：

```text
== 9. auto ==
a = 42, r = 1, r2 = 1, y = 42, *p = 1, sizeof(arr_ref) = 20
```

- `a` 改成 42，但 `ci`、`r`、`r2` 都不受影响；
- `*p` 是 `arr[0]`，值为 1；
- `sizeof(arr_ref)` 是 20，证明引用没有退化。

## 易错点

1. **`const int*` 不是 const 指针**：它是指向 const int 的指针，指针本身可以改指向。
2. **`int* const` 必须初始化**：const 对象没有默认初始化。
3. **`const` ≠ `constexpr`**：`const` 可能只是运行期只读；`constexpr` 要求编译期可求值。
4. **`constexpr` 函数也能运行时调用**：不要以为写了 `constexpr` 就只能在编译期用。
5. **`constexpr` 函数里不能有 I/O**：`std::cout` 调用会让它无法用于常量表达式；GCC 会给 `-Winvalid-constexpr`。
6. **`auto` 丢顶层 const 和引用**：`auto a = ci;` 的 `a` 是 `int`，可以再赋值；想保留 const 用 `const auto` / `auto&`。
7. **`auto` 会让数组退化**：`auto p = arr;` 是 `int*`；想保留数组用 `auto&`。
8. **`typeid` 会剥引用和顶层 const**：不要用它验证 `const int&` 这类类型，用 `static_assert`。
9. **顶层 const 与底层 const 的拷贝行为不同**：`auto q = cp;` 会丢掉 `cp` 的顶层 const，但 `auto q = pc;` 会保留 `pc` 指向对象的 const。

## 待复习

- 从右往左读 `const int*` / `int* const` / `const int* const`。
- 顶层 const 与底层 const 的区别；拷贝/`auto` 时分别怎样。
- `const`、`constexpr`、`consteval` 三者的区别。
- `constexpr` 函数的编译期求值条件；为什么含 `std::cout` 的函数不能用于常量表达式。
- `auto` 的按值推导规则：顶层 const、引用、数组退化。
- `auto&` 为什么能保留 const 和数组类型。
- `decltype(expr)` 与 `decltype((expr))` 的区别。
- `typeid` 为什么会剥掉引用和顶层 const。
