# D03 任务 2：命名空间、`static`、匿名 namespace

> 环境：WSL2 Ubuntu 24.04，g++ 13.3.0，CMake 3.28.3，Ninja。
> 代码目录：[code/](code/)
>
> 任务 2.1–2.5 的代码都已补齐，并实际编译、链接、运行；两个故意失败的链接实验也实际复现了 `multiple definition`。下面的输出都来自这次运行，不是凭记忆写的“应该怎样”。

## 0. 速查

- `namespace math { ... }` 把名字放进作用域，外部用 `math::add` 访问；嵌套用 `math::detail::twice`。
- `static` 修饰**命名空间作用域**的变量/函数 → 内部链接（internal linkage），只在本 TU 可见。
- 匿名 namespace 里的名字同样是内部链接；它比 `static` 多一个好处：可以包类型、函数、变量。
- `nm` 常见对应关系：`T/t` 函数、`D/d` 已初始化数据、`B/b` BSS、`R/r` 只读数据；**大写通常是外部链接，小写通常是局部链接**。
- 但有例外：`U` 是未定义，`W` 是弱函数，`V/v` 是弱对象，`u`（小写）是 GNU 的 unique global symbol，不是“局部”。
- `using math::add;` 只引入一个名字；`using namespace math;` 把整个命名空间的名字都拉进当前作用域。
- 头文件里不要写 `using namespace ...;`，会污染每个包含它的 TU。
- namespace 作用域的 `const` 默认内部链接；`extern const` 带初始化器是定义；`inline constexpr`（C++17）是允许多 TU 定义的变量。
- `inline` 函数可以放头文件，链接器会把多个 TU 的相同定义合并；`#pragma once` 只防同一个 TU 重复包含，不解决跨 TU 的 ODR 冲突。

## 1. 文件清单

| 文件 | 用途 | 结果 |
|---|---|---|
| `namespaces.cpp` | 2.1 命名空间、`using`；2.2 `static` / 匿名 namespace | 编译运行成功 |
| `link_internal.h` | 2.3 两个 TU 的外部函数声明 | — |
| `link_internal_a.cpp` | 2.3 a 侧 `static counter`、匿名 `helper` | 与 b 链接成功 |
| `link_internal_b.cpp` | 2.3 b 侧同名 `static counter`、匿名 `helper` | 与 a 链接成功 |
| `link_internal_main.cpp` | 2.3 调用 `a_value` / `b_value` | 运行成功 |
| `const_linkage.h` | 2.4 `const` / `extern const` / `inline constexpr` 声明与定义 | — |
| `const_linkage_a.cpp` | 2.4 a 侧使用三种常量 | 链接成功 |
| `const_linkage_b.cpp` | 2.4 b 侧使用三种常量 | 链接成功 |
| `const_linkage_def.cpp` | 2.4 `kExternal` 的唯一外部定义 | 链接成功 |
| `const_linkage_main.cpp` | 2.4 打印值、地址、地址是否相同 | 运行成功 |
| `link_inline.h` | 2.5 `inline int twice(int)` | 两个 TU 包含后链接成功 |
| `link_inline_a.cpp` / `link_inline_b.cpp` / `link_inline_main.cpp` | 2.5 两个 TU + main 都包含头文件 | 链接成功 |
| `fail_extern_const.h` | 2.4 故意失败：头文件里定义 `extern const` | 链接失败 |
| `fail_extern_const_a.cpp` / `_b.cpp` / `_main.cpp` | 2.4 故意失败：三个 TU 都包含该头文件 | `multiple definition` |
| `fail_link_inline.h` | 2.5 故意失败：去掉 `inline` 的函数定义放头文件 | 链接失败 |
| `fail_link_inline_a.cpp` / `_b.cpp` / `_main.cpp` | 2.5 故意失败：三个 TU 都包含该头文件 | `multiple definition` |

`fail_*` 文件只用来记录链接错误，不要加入 CMake target。

## 2. 构建与运行

统一编译选项：

```bash
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror"
```

### 2.1 `namespaces.cpp`

```bash
g++ $FLAGS -c namespaces.cpp -o /tmp/d03_task2/namespaces.o
g++ /tmp/d03_task2/namespaces.o -o /tmp/d03_task2/namespaces
/tmp/d03_task2/namespaces
```

实际输出：

```text
Sum: 7, Double Sum: 14
using declaration: 3
using directive: 5
Hidden counter: 1
Hidden counter: 2
```

> **记忆点：** `namespace` 管“名字在哪”，`static` / 匿名 namespace 管“名字给不给别的 TU 看”，`using` 只管当前作用域。

### 2.2 两个 TU 的同名内部符号

```bash
g++ $FLAGS -c link_internal_a.cpp -o /tmp/d03_task2/link_internal_a.o
g++ $FLAGS -c link_internal_b.cpp -o /tmp/d03_task2/link_internal_b.o
g++ $FLAGS -c link_internal_main.cpp -o /tmp/d03_task2/link_internal_main.o
g++ /tmp/d03_task2/link_internal_a.o \
    /tmp/d03_task2/link_internal_b.o \
    /tmp/d03_task2/link_internal_main.o \
    -o /tmp/d03_task2/link_internal
/tmp/d03_task2/link_internal
```

实际输出：

```text
a_value=42, b_value=42
after a_increment: a_value=43, b_value=42
after b_increment twice: a_value=43, b_value=44
```

两个 TU 各自有一份 `counter`：增加 a 的计数器不影响 b，反过来也一样。

> **记忆点：** 两个 `.cpp` 可以各写一份同名 `static` / 匿名 namespace 实体；它们各自私有，链接器不会撞。

### 2.3 `const` 链接属性

```bash
g++ $FLAGS -c const_linkage_a.cpp -o /tmp/d03_task2/const_linkage_a.o
g++ $FLAGS -c const_linkage_b.cpp -o /tmp/d03_task2/const_linkage_b.o
g++ $FLAGS -c const_linkage_def.cpp -o /tmp/d03_task2/const_linkage_def.o
g++ $FLAGS -c const_linkage_main.cpp -o /tmp/d03_task2/const_linkage_main.o
g++ /tmp/d03_task2/const_linkage_a.o \
    /tmp/d03_task2/const_linkage_b.o \
    /tmp/d03_task2/const_linkage_def.o \
    /tmp/d03_task2/const_linkage_main.o \
    -o /tmp/d03_task2/const_linkage
/tmp/d03_task2/const_linkage
```

实际输出（某次运行）：

```text
kShared: a=7, b=7
kExternal: a=7, b=7
kInline: 7
a kShared addr=0x602f1638a004, b kShared addr=0x602f1638a00c
a kInline addr=0x602f1638a008, b kInline addr=0x602f1638a008
kShared same address? false
kInline same address? true
```

具体地址值会随链接布局/ASLR 变化，**要看的是**：`kShared` 在 a、b 中地址不同，`kInline` 地址相同。

> **记忆点：** `const` 默认一个 TU 一份，`extern const` 全程序一份，`inline constexpr` 多份定义合并成一份。

### 2.4 `inline` 函数放头文件

```bash
g++ $FLAGS -c link_inline_a.cpp -o /tmp/d03_task2/link_inline_a.o
g++ $FLAGS -c link_inline_b.cpp -o /tmp/d03_task2/link_inline_b.o
g++ $FLAGS -c link_inline_main.cpp -o /tmp/d03_task2/link_inline_main.o
g++ /tmp/d03_task2/link_inline_a.o \
    /tmp/d03_task2/link_inline_b.o \
    /tmp/d03_task2/link_inline_main.o \
    -o /tmp/d03_task2/link_inline
/tmp/d03_task2/link_inline
```

实际输出：

```text
inline_a=42, inline_b=20, main twice(3)=6
```

三个 TU 都包含 `link_inline.h`，都有 `twice` 的定义，链接成功。

> **记忆点：** 头文件里定义函数要加 `inline`；`#pragma once` 只防同一个 TU 重复包含，防不了跨 TU 重复定义。

## 3. 命名空间

### 3.1 定义与访问

```cpp
namespace math {
    int add(int a, int b) { return a + b; }

    namespace detail {
        int twice(int x) { return 2 * x; }
    }
}
```

`math` 是命名空间名，`add` 和 `detail` 是 `math` 的成员；`twice` 是 `math::detail` 的成员。

访问方式：

```cpp
math::add(3, 4);
math::detail::twice(7);
```

`::` 是作用域解析运算符（scope resolution operator），从当前作用域逐层向外找，或者从全局命名空间开始找（`::math::add`）。

### 3.2 嵌套 namespace 的 C++17 写法

C++17 起可以写成：

```cpp
namespace math::detail {
    int twice(int x) { return 2 * x; }
}
```

它和 `namespace math { namespace detail { ... } }` 等价。当前代码用了传统嵌套写法，便于看清层级。

`detail` 是常见约定，表示“实现细节，外部不要依赖”。

### 3.3 `using` 声明 vs `using` 指令

当前 `namespaces.cpp` 在 `main` 里放了两个块作用域：

```cpp
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
```

实际输出：

```text
using declaration: 3
using directive: 5
```

区别：

- `using math::add;` 是 **using-declaration**，只引入 `add` 一个名字；`detail` 不会自动可见。
- `using namespace math;` 是 **using-directive**，把 `math` 里的所有名字都引入当前作用域；`add`、`detail` 都可以无限定访问。
- 两者都可以放在函数/块作用域，减少污染范围。
- **头文件里不要写 `using namespace ...;`**：每个包含该头文件的 TU 都会被污染，容易出现名字冲突和“到底用了哪个 `size`/`count`”的问题。头文件里用全限定名或 `using` 声明。

## 4. `static` 与匿名 namespace

### 4.1 `static` 在命名空间作用域

```cpp
[[maybe_unused]] static int file_local = 7;
```

这里的 `static` 不是“静态存储期”的意思（命名空间作用域变量本来就有静态存储期），而是**内部链接（internal linkage）**：

- 名字只在当前翻译单元（TU）可见；
- 不参与跨 TU 的符号解析；
- 另一个 `.cpp` 也可以定义自己的 `file_local`，链接器不会报 multiple definition。

### 4.2 匿名 namespace

```cpp
namespace {
    int hidden_counter = 0;
    void bump() { ++hidden_counter; }
}
```

匿名 namespace 等价于给内部名字生成一个唯一命名空间，并对该命名空间写 `using namespace`。效果：

- 里面的名字只在本 TU 可见，内部链接；
- 可以包含变量、函数、类型、模板等；
- 比 `static` 更通用：`static` 不能修饰类型，匿名 namespace 可以。

C++ 里通常优先用匿名 namespace 表达“这个 TU 私有的实现细节”。

### 4.3 和局部 `static` 的区别

```cpp
void f() {
    static int calls = 0;  // 局部 static：静态存储期，只初始化一次
    ++calls;
}
```

这是另一个概念：局部 `static` 变量生命周期贯穿程序，但作用域仍在函数内。今天讨论的是**命名空间作用域的 `static`**（内部链接）。局部 `static` 的初始化/析构顺序留到 D4 再展开。

## 5. 用 `nm` 看命名空间符号

### 5.1 `nm -C` 的实际输出（相关行）

```text
0000000000000000 d file_local
0000000000000000 b (anonymous namespace)::hidden_counter
000000000000002a t (anonymous namespace)::bump()
0000000000000000 T math::add(int, int)
0000000000000018 T math::detail::twice(int)
0000000000000082 T main
```

`-C` 是 demangle：把编译器修饰过的符号名还原成人类可读的 C++ 名字。

### 5.2 字母含义

| 字母 | 含义 | 链接属性 |
|---|---|---|
| `T` | text 段中的函数 | 外部（global） |
| `t` | text 段中的局部函数 | 内部（local） |
| `D` | 已初始化数据段 | 外部 |
| `d` | 已初始化数据段 | 内部 |
| `B` | BSS 段（未初始化/零初始化） | 外部 |
| `b` | BSS 段 | 内部 |
| `R` | 只读数据段 | 外部 |
| `r` | 只读数据段 | 内部 |
| `W` | 弱函数（weak） | 多个定义可合并 |
| `V` / `v` | 弱对象 | 多个定义可合并 |
| `U` | undefined，未定义 | 等链接器解析 |
| `u` | unique global symbol（GNU） | 多个定义合并为一个 |

注意：不要把所有小写都当成“局部”。`u` 是 unique global，`W` 是弱符号；`U` 和 `u` 也不是一回事。

所以：

- `math::add`、`math::detail::twice`、`main` 是 `T`：外部链接，其他 TU 可以引用；
- `file_local` 是 `d`：内部链接；
- `(anonymous namespace)::hidden_counter` 是 `b`：内部链接的 BSS；
- `(anonymous namespace)::bump()` 是 `t`：内部链接的函数；
- `std::cout`、`operator<<`、`endl` 在本 TU 的目标文件里是 `U`：只声明/使用，定义在 libstdc++ 里。

### 5.3 原始（mangled）符号

```text
0000000000000000 d _ZL10file_local
0000000000000000 b _ZN12_GLOBAL__N_114hidden_counterE
000000000000002a t _ZN12_GLOBAL__N_14bumpEv
0000000000000000 T _ZN4math3addEii
0000000000000018 T _ZN4math6detail5twiceEi
0000000000000082 T main
```

以 Itanium C++ ABI 为例：

- `_Z` 是 C++ 修饰名的前缀；
- `N ... E` 表示嵌套名字；
- `4math` 表示长度为 4 的 `math`，`3add` 表示长度为 3 的 `add`；
- `ii` 表示两个 `int` 参数；
- `_ZL10file_local` 里的 `L` 表示内部链接；
- `_ZN12_GLOBAL__N_1...` 里的 `_GLOBAL__N_1` 是匿名 namespace 的内部编码。

`nm` 不加 `-C` 时看到的就是这些名字；链接器实际处理的是它们。不同编译器/平台的修饰规则可能不同（MSVC 是另一套），不要手写 mangled name。

> **记忆点：** `nm -C` 看可读名；小写多为局部、大写多为全局，但 `U`（未定义）、`u`（UNIQUE）、`W`（WEAK）要单独记。

## 6. 任务 2.3：两个 TU 里的同名内部符号

### 6.1 代码

`link_internal_a.cpp`：

```cpp
#include "link_internal.h"

// static 命名空间作用域变量：内部链接，只属于本 TU。
static int counter = 0;

namespace {
// 匿名 namespace：helper 也是内部链接，只属于本 TU。
int helper() {
    return 42;
}
}  // namespace

int a_value() {
    return counter + helper();
}

void a_increment() {
    ++counter;
}
```

`link_internal_b.cpp` 结构相同，只是外部函数换成 `b_value` / `b_increment`：

```cpp
#include "link_internal.h"

static int counter = 0;

namespace {
int helper() {
    return 42;
}
}  // namespace

int b_value() {
    return counter + helper();
}

void b_increment() {
    ++counter;
}
```

两个文件都有同名的 `counter` 和 `helper`。它们能链接在一起，是因为两者都是内部链接。

### 6.2 目标文件符号

```text
===== link_internal_a.o =====
0000000000000026 T a_increment()
000000000000000f T a_value()
0000000000000000 b counter
0000000000000000 t (anonymous namespace)::helper()

===== link_internal_b.o =====
0000000000000026 T b_increment()
000000000000000f T b_value()
0000000000000000 b counter
0000000000000000 t (anonymous namespace)::helper()
```

- `counter` 是 `b`：BSS 中的局部符号（零初始化，内部链接）；
- `helper()` 是 `t`：text 中的局部函数；
- `a_value` / `b_value` 是 `T`：外部链接，所以 main 能调用。

两个 `.o` 里都有 `counter` 和 `helper`，但都是小写局部符号，链接器不会把它们放进全局符号表做冲突检查。

### 6.3 链接后的可执行文件

```text
0000000000001198 T a_value()
00000000000011d8 T b_value()
0000000000004154 b counter
0000000000004158 b counter
0000000000001189 t (anonymous namespace)::helper()
00000000000011c9 t (anonymous namespace)::helper()
```

可执行文件里保留了**两份** `counter`（地址 `0x4154`、`0x4158`）和**两份** `helper`，分别属于 a、b 的代码。运行结果也验证了它们互不影响：

```text
a_value=42, b_value=42
after a_increment: a_value=43, b_value=42
after b_increment twice: a_value=43, b_value=44
```

### 6.4 为什么不会 multiple definition

链接器只把**外部链接**的符号放进全局符号表进行匹配。`static` 和匿名 namespace 让 `counter` / `helper` 成为各自 TU 的局部符号：

- 名字不导出；
- 不同 TU 可以重复使用同一个名字；
- 每个 TU 各有一份存储/代码。

所以不会冲突。反过来，如果把 `counter` 写成普通的 `int counter = 0;`，两个 TU 就会各导出一个 `B counter`，链接时就会 `multiple definition of 'counter'`。

## 7. 任务 2.4：`const` 的链接属性

### 7.1 代码

`const_linkage.h`：

```cpp
#pragma once

// namespace 作用域的 const 默认内部链接：每个包含本头文件的 TU 各有一份。
const int kShared = 7;

// 只声明，不定义；定义放在 const_linkage_def.cpp 里，只出现一次。
extern const int kExternal;

// C++17 inline variable：外部链接，但允许多个 TU 有相同定义，链接器合并。
inline constexpr int kInline = 7;

int shared_a();
int shared_b();
int external_a();
int external_b();
const int* shared_a_addr();
const int* shared_b_addr();
const int* inline_a_addr();
const int* inline_b_addr();
```

`const_linkage_a.cpp`（b 侧对称）：

```cpp
#include "const_linkage.h"

int shared_a() {
    return kShared;
}

int external_a() {
    return kExternal;
}

const int* shared_a_addr() {
    return &kShared;
}

const int* inline_a_addr() {
    return &kInline;
}
```

`const_linkage_def.cpp`：

```cpp
#include "const_linkage.h"

// 整个工程里唯一的 kExternal 定义。
extern const int kExternal = 7;
```

### 7.2 `nm` 看三种常量

`const_linkage_a.o`：

```text
0000000000000000 r kShared
                 U kExternal
0000000000000000 u kInline
```

`const_linkage_b.o` 相同。`const_linkage_def.o`：

```text
0000000000000004 R kExternal
```

解释：

- `kShared` 是 `r`：只读数据段中的**局部**符号。每个包含 `const_linkage.h` 的 TU 都有一份，内部链接，不会冲突。
- `kExternal` 在 a/b 中是 `U`（未定义），在 def 中是 `R`（外部只读定义）。头文件只声明，定义只出现一次。
- `kInline` 是 `u`：GNU 的 unique global symbol。每个 TU 都可以有一份，链接器最终只保留一份。

可执行文件里更直观：

```text
0000000000002004 r kShared
000000000000200c r kShared
0000000000002010 r kShared
0000000000002018 r kShared
0000000000002014 R kExternal
0000000000002008 u kInline
```

有 **4 份** `kShared`，因为 `const_linkage_a.cpp`、`b.cpp`、`def.cpp`、`main.cpp` 都包含了头文件；每个 TU 各一份，地址不同：

```text
kShared same address? false
kInline same address? true
```

`kInline` 在目标文件里是 `u`；`readelf -sW` 显示它的绑定是 `UNIQUE`：

```text
5: 0000000000000000     4 OBJECT  UNIQUE DEFAULT    7 kInline
```

### 7.3 为什么两个 TU 包含定义 `kShared` 的头文件不报错

C++ 规定：命名空间作用域的 `const` 变量，如果没有显式 `extern`、没有 `inline`，默认是**内部链接**。所以：

```cpp
const int kShared = 7;  // 每个 TU 各一份，互不干扰
```

这不是“链接器帮忙合并”，而是每个 TU 本来就有独立的实体。地址不同就是证据。

### 7.4 故意失败：头文件里定义 `extern const`

`fail_extern_const.h`：

```cpp
#pragma once

// 故意失败：带初始化器的 extern const 是定义，不是声明。
extern const int kExternalBad = 7;
```

`fail_extern_const_a.cpp`、`_b.cpp`、`_main.cpp` 都包含它。实际链接错误：

```text
/usr/bin/ld: /tmp/d03_task2/fail_extern_const_b.o:(.rodata+0x0): multiple definition of `kExternalBad'; /tmp/d03_task2/fail_extern_const_a.o:(.rodata+0x0): first defined here
/usr/bin/ld: /tmp/d03_task2/fail_extern_const_main.o:(.rodata+0x0): multiple definition of `kExternalBad'; /tmp/d03_task2/fail_extern_const_a.o:(.rodata+0x0): first defined here
collect2: error: ld returned 1 exit status
```

原因：`extern const int kExternalBad = 7;` 带初始化器，是**定义**，而且是外部链接。三个 TU 各生成一份 `kExternalBad`，链接器看到三个全局定义。

> **记忆点：** 头文件里的 `extern const T x = ...;` 是定义，不是声明；被几个 TU 包含就定义几份。

正确写法：

```cpp
// 头文件：只声明
extern const int kExternal;

// 某一个 .cpp：定义一次
extern const int kExternal = 7;
```

### 7.5 三种写法对比

| 写法 | 链接属性 | 能否放头文件被多个 TU 包含 | 说明 |
|---|---|---|---|
| `const int kShared = 7;` | 内部 | 可以 | 每个 TU 各一份，地址不同 |
| `extern const int kExternal = 7;` | 外部 | **不可以** | 带初始化器就是定义，多个 TU 会重复定义 |
| `extern const int kExternal;` | 外部声明 | 可以 | 定义必须放到某个 `.cpp` 里一次 |
| `inline constexpr int kInline = 7;` | 外部，可合并 | 可以 | C++17 inline variable，链接器保留一份 |

## 8. 任务 2.5：`inline` 函数放头文件

### 8.1 代码

`link_inline.h`：

```cpp
#pragma once

// inline 函数允许在多个 TU 中出现相同定义，链接器会合并。
inline int twice(int x) {
    return x * 2;
}
```

三个 TU 都包含它：

```cpp
// link_inline_a.cpp
#include "link_inline.h"
int inline_a() { return twice(21); }
```

```cpp
// link_inline_b.cpp
#include "link_inline.h"
int inline_b() { return twice(10); }
```

```cpp
// link_inline_main.cpp
#include "link_inline.h"
#include <iostream>

int inline_a();
int inline_b();

int main() {
    std::cout << "inline_a=" << inline_a()
              << ", inline_b=" << inline_b()
              << ", main twice(3)=" << twice(3) << '\n';
    return 0;
}
```

实际输出：

```text
inline_a=42, inline_b=20, main twice(3)=6
```

### 8.2 符号表

三个目标文件里都有 `twice(int)`，但都是 `W`（weak）：

```text
===== link_inline_a.o =====
0000000000000000 W twice(int)
0000000000000000 T inline_a()

===== link_inline_b.o =====
0000000000000000 W twice(int)
0000000000000000 T inline_b()

===== link_inline_main.o =====
0000000000000000 W twice(int)
0000000000000000 T main
```

`readelf -sW link_inline_a.o` 显示 `twice` 的绑定是 `WEAK`：

```text
4: 0000000000000000    18 FUNC    WEAK   DEFAULT    6 _Z5twicei
```

链接后的可执行文件只剩一个 `twice(int)`：

```text
000000000000119d W twice(int)
```

这就是 `inline` 的作用：允许多个 TU 有相同定义，链接器选择/合并其中一份。

> **记忆点：** `inline` 函数在 `.o` 里是 `W`（WEAK），`inline` 变量是 `u`（UNIQUE）；两者都表示“多个定义可以合并”。

### 8.3 故意失败：去掉 `inline`

`fail_link_inline.h`：

```cpp
#pragma once

// 故意失败：去掉 inline 后，函数定义放进头文件，
// 被多个 TU 包含时会生成多份外部链接定义。
int twice(int x) {
    return x * 2;
}
```

三个 TU 都包含它。实际链接错误：

```text
/usr/bin/ld: /tmp/d03_task2/fail_link_inline_b.o: in function `twice(int)':
fail_link_inline_b.cpp:(.text+0x0): multiple definition of `twice(int)'; /tmp/d03_task2/fail_link_inline_a.o:fail_link_inline_a.cpp:(.text+0x0): first defined here
/usr/bin/ld: /tmp/d03_task2/fail_link_inline_main.o: in function `twice(int)':
fail_link_inline_main.cpp:(.text+0x0): multiple definition of `twice(int)'; /tmp/d03_task2/fail_link_inline_a.o:fail_link_inline_a.cpp:(.text+0x0): first defined here
collect2: error: ld returned 1 exit status
```

去掉 `inline` 后，`twice` 是普通外部链接函数；每个 TU 都生成一份 `T twice(int)`，链接器看到重复定义。

> **记忆点：** 去掉 `inline`，头文件里的函数定义就变成每个 TU 一份的强符号，链接器必然报 `multiple definition`。

### 8.4 `#pragma once` 为什么不够

- `#pragma once` 或 include guard 只保证**同一个 TU** 不会重复展开头文件；
- 它管不到两个不同的 TU 都包含同一个头文件、各自生成一份函数定义；
- `inline` 是语言层面的“允许多个相同定义”的机制，解决的是跨 TU 的 ODR 问题。

所以头文件里定义函数时要写 `inline`（或者用模板、`constexpr` 等本身就允许的机制），不能只靠 `#pragma once`。

## 9. 本次实际遇到的一个警告

第一次编译 `namespaces.cpp` 时，`file_local` 没有被使用：

```text
namespaces.cpp:3:12: warning: ‘file_local’ defined but not used [-Wunused-variable]
    3 | static int file_local = 7;
      |            ^~~~~~~~~~
```

`-Wall` 会打开 `-Wunused-variable`；在 `-Werror` 下这会直接变成错误。修复方式是加 `[[maybe_unused]]`：

```cpp
[[maybe_unused]] static int file_local = 7;
```

它只抑制“未使用”的警告，不改变链接属性；`nm` 里仍然是 `d file_local`。

## 10. 易错点

1. **把 `static` 只理解成“静态变量”**：命名空间作用域的 `static` 重点是内部链接；局部 `static` 才是静态存储期。
2. **以为匿名 namespace 有外部链接**：匿名 namespace 里的名字是内部链接，`nm` 里是小写。
3. **在头文件里写 `using namespace std;` 或 `using namespace math;`**：污染所有包含者；头文件用全限定名或 `using` 声明。
4. **把 `#pragma once` 当成 ODR 修复**：它只防同一 TU 重复包含，跨 TU 的重复定义要靠 `inline` 或“头文件只放声明”。
5. **以为 `const int k = 7;` 一定是外部链接**：命名空间作用域的 `const` 默认内部链接。
6. **把 `extern const int k = 7;` 放头文件**：这是定义，多个 TU 包含会重复定义。
7. **把 `inline constexpr` 当成普通 `const`**：`inline` 变量是外部链接、可合并；普通 `const` 是每个 TU 一份。
8. **误读 `nm` 的大小写**：`T/t`、`D/d` 等可以按大小写区分内外链接，但 `U`、`u`、`W`、`V` 有专门含义，不能简单套“小写就是局部”。
9. **以为 `U` 和 `u` 一样**：`U` 是未定义，`u` 是 unique global symbol。
10. **手写 mangled name**：不同编译器/平台规则不同，用 `nm -C` 或 `c++filt` 即可。
11. **忽略未使用变量警告**：`-Wunused-variable` 在严格构建里会挡住你；要么使用变量，要么明确 `[[maybe_unused]]`。

## 11. 完整复现与验收

在 `code/` 目录下：

```bash
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror"
mkdir -p /tmp/d03_task2

# 2.1 / 2.2
g++ $FLAGS -c namespaces.cpp -o /tmp/d03_task2/namespaces.o
g++ /tmp/d03_task2/namespaces.o -o /tmp/d03_task2/namespaces

# 2.3
g++ $FLAGS -c link_internal_a.cpp -o /tmp/d03_task2/link_internal_a.o
g++ $FLAGS -c link_internal_b.cpp -o /tmp/d03_task2/link_internal_b.o
g++ $FLAGS -c link_internal_main.cpp -o /tmp/d03_task2/link_internal_main.o
g++ /tmp/d03_task2/link_internal_a.o /tmp/d03_task2/link_internal_b.o \
    /tmp/d03_task2/link_internal_main.o -o /tmp/d03_task2/link_internal

# 2.4
g++ $FLAGS -c const_linkage_a.cpp -o /tmp/d03_task2/const_linkage_a.o
g++ $FLAGS -c const_linkage_b.cpp -o /tmp/d03_task2/const_linkage_b.o
g++ $FLAGS -c const_linkage_def.cpp -o /tmp/d03_task2/const_linkage_def.o
g++ $FLAGS -c const_linkage_main.cpp -o /tmp/d03_task2/const_linkage_main.o
g++ /tmp/d03_task2/const_linkage_a.o /tmp/d03_task2/const_linkage_b.o \
    /tmp/d03_task2/const_linkage_def.o /tmp/d03_task2/const_linkage_main.o \
    -o /tmp/d03_task2/const_linkage

# 2.5
g++ $FLAGS -c link_inline_a.cpp -o /tmp/d03_task2/link_inline_a.o
g++ $FLAGS -c link_inline_b.cpp -o /tmp/d03_task2/link_inline_b.o
g++ $FLAGS -c link_inline_main.cpp -o /tmp/d03_task2/link_inline_main.o
g++ /tmp/d03_task2/link_inline_a.o /tmp/d03_task2/link_inline_b.o \
    /tmp/d03_task2/link_inline_main.o -o /tmp/d03_task2/link_inline

/tmp/d03_task2/namespaces
/tmp/d03_task2/link_internal
/tmp/d03_task2/const_linkage
/tmp/d03_task2/link_inline
```

验收标准：

- [x] `namespaces.cpp` 零警告，输出包含 `using declaration` 和 `using directive`；
- [x] `link_internal_a.o` / `link_internal_b.o` 中 `counter`、`helper` 都是局部符号，链接不冲突；
- [x] `link_internal` 的两个计数器互不影响；
- [x] `kShared` 在每个 TU 各一份、地址不同；`kExternal` 只有一份定义；`kInline` 地址相同；
- [x] `link_inline.h` 被三个 TU 包含后链接成功；
- [x] `fail_extern_const` 和 `fail_link_inline` 都实际复现 `multiple definition`；
- [ ] 把以上正常文件加入 CMake target（留给任务 6）。

> 说明：`fail_extern_const_*`、`fail_link_inline_*` 是故意失败文件，不加入 CMake，也不要在主构建里引用。

## 12. 待复习

- `namespace` 的嵌套、全限定名、作用域解析运算符。
- `using` 声明和 `using namespace` 指令的区别；为什么头文件里不要用后者。
- 命名空间作用域 `static` 的链接属性；局部 `static` 的区别。
- 匿名 namespace 的链接属性和适用场景。
- `nm` 中 `T/t`、`D/d`、`B/b`、`R/r`、`W`、`U`、`u` 的含义。
- `nm -C` 和原始 mangled name 的关系。
- 两个 TU 各自定义 `static int counter` 为什么不会冲突？可执行文件里有几份？
- `const`、`extern const`、`inline constexpr` 的链接属性，以及哪种能放头文件。
- `#pragma once` 能解决什么、不能解决什么；`inline` 函数为什么能放头文件。
- `inline` 函数在目标文件里是 `W`（WEAK），`inline` 变量是 `u`（UNIQUE），有什么区别。
