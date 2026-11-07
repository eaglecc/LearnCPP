# D03 任务 3：多文件工程与编译/链接错误

> 环境：WSL2 Ubuntu 24.04，g++ 13.3.0，CMake 3.28.3，Ninja。
> 代码目录：[code/](code/)
>
> 本笔记记录的是实际编译、链接、运行和失败实验的结果。正常文件使用：
> `-std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror`

## 0. 速查

- 每个 `.cpp` 经过预处理后形成一个 **TU**，单独编译成一个 `.o`。
- 编译器只检查当前 TU；跨 TU 的符号由链接器解析。
- `nm` 中 `U` 表示本目标文件里“未定义”，`T` 表示已定义，`W` 表示弱定义可合并。
- `undefined reference`：有声明、有调用，但所有 `.o` 里都没有定义。
- `multiple definition`：同一个外部链接符号在多个 TU 里都有定义。
- `#pragma once` 只防同一个 TU 重复包含，防不了多个 TU 各包含一次。
- 修复 `multiple definition` 的两种常见方式：
  1. 头文件只放声明，定义放一个 `.cpp`；
  2. 函数写在头文件里，但加上 `inline`。
- 类内直接定义的成员函数隐式 `inline`；普通自由函数不是。
- 编译错误是“这个 TU 自己有问题”；链接错误是“每个 TU 都编译过了，但符号拼不起来”。

## 1. 文件清单

| 文件 | 用途 | 结果 |
|---|---|---|
| `link_stats.h` | 3.1 只放 `sum` / `average` 声明 | — |
| `link_stats.cpp` | 3.1 实现 `sum` / `average` | 编译成功 |
| `link_main.cpp` | 3.1 `main`，调用两个函数 | 链接运行成功 |
| `fail_undefined.cpp` | 3.2 只声明 `missing_function`，不定义 | 编译成功，链接失败 |
| `fail_dup.h` | 3.3 普通函数定义放头文件 | — |
| `fail_dup_a.cpp` | 3.3 包含 `fail_dup.h`，写 `main` | 编译成功 |
| `fail_dup_b.cpp` | 3.3 包含 `fail_dup.h`，写另一个函数 | 编译成功 |
| `fix_dup_decl.h` | 3.3 修复 1：只放声明 | — |
| `fix_dup_def.cpp` | 3.3 修复 1：唯一的 `twice` 定义 | 链接成功 |
| `fix_dup_main.cpp` | 3.3 修复 1：`main` | 运行成功 |
| `fix_dup_other.cpp` | 3.3 修复 1：另一个调用 `twice` 的 TU | 运行成功 |
| `fix_dup_inline.h` | 3.3 修复 2：`inline` 函数放头文件 | — |
| `fix_dup_inline_main.cpp` | 3.3 修复 2：`main` | 链接成功 |
| `fix_dup_inline_other.cpp` | 3.3 修复 2：另一个 TU | 链接成功 |
| `fail_syntax.cpp` | 3.4 缺少分号 | 编译失败 |
| `fail_narrow.cpp` | 3.4 列表初始化窄化 | 编译失败 |

`fail_*` 文件只用于复现错误，不要加入 CMake target。

## 2. 任务 3.1：正常的多文件工程

### 2.1 代码

`link_stats.h` 只放声明：

```cpp
#pragma once

// 只放声明，不放函数体。
int sum(const int* data, int n);
double average(const int* data, int n);
```

`link_stats.cpp` 放定义：

```cpp
#include "link_stats.h"

int sum(const int* data, int n) {
    int total = 0;
    for (int i = 0; i < n; ++i) {
        total += data[i];
    }
    return total;
}

double average(const int* data, int n) {
    if (n <= 0) {
        return 0.0;
    }
    return static_cast<double>(sum(data, n)) / static_cast<double>(n);
}
```

`link_main.cpp` 只包含头文件：

```cpp
#include "link_stats.h"

#include <iostream>

int main() {
    const int data[] = {4, 8, 15, 16, 23, 42};
    constexpr int n = 6;

    std::cout << "sum=" << sum(data, n) << '\n';
    std::cout << "average=" << average(data, n) << '\n';

    return 0;
}
```

### 2.2 分别编译

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -c link_stats.cpp -o /tmp/d03_task3/link_stats.o
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -c link_main.cpp -o /tmp/d03_task3/link_main.o
```

两个 `.cpp` 都能单独编译成功，各自得到一个 `.o`。

### 2.3 看目标文件符号

`link_main.o`：

```text
                 U sum(int const*, int)
                 U average(int const*, int)
```

`link_stats.o`：

```text
0000000000000000 T sum(int const*, int)
0000000000000049 T average(int const*, int)
```

解释：

- `link_main.o` 只有声明和调用，没有函数体，所以是 `U`（undefined）；
- `link_stats.o` 有函数体，所以是 `T`（text 段中的全局定义）；
- 这里的 `U` 不是错误，只是一个“待链接器解析”的引用。

### 2.4 链接并运行

```bash
g++ /tmp/d03_task3/link_main.o /tmp/d03_task3/link_stats.o \
    -o /tmp/d03_task3/d03_link
/tmp/d03_task3/d03_link
```

最终可执行文件里，两个符号都变成了 `T`：

```text
00000000000012c5 T sum(int const*, int)
000000000000130e T average(int const*, int)
```

运行输出：

```text
sum=108
average=18
```

> **记忆点：** 编译只看单个 TU，链接才看全局符号；`.o` 里的 `U` 要靠其他 `.o` 的 `T` 来补。

### 2.5 为什么必须分成声明和定义

- 编译器编译 `link_main.cpp` 时，只看到 `link_stats.h` 里的声明，知道 `sum` 的参数和返回类型，足以生成调用代码；
- 但编译器不知道 `sum` 的地址，也不知道它在哪个 `.o` 里，所以先留下一个未定义符号；
- 链接器把所有 `.o` 放在一起，用 `link_stats.o` 里的 `T sum` 去补 `link_main.o` 里的 `U sum`；
- 如果头文件里直接写函数体，每个包含它的 TU 都会生成一份定义，反而可能造成 `multiple definition`。

> **记忆点：** 头文件负责“让调用者知道怎么调用”，`.cpp` 负责“真正实现”。

## 3. 任务 3.2：复现 `undefined reference`

### 3.1 代码

`fail_undefined.cpp`：

```cpp
// 故意失败：只声明，不提供定义。
// 编译能通过，链接会报 undefined reference。
int missing_function();

int main() {
    return missing_function();
}
```

### 3.2 编译成功

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -c fail_undefined.cpp -o /tmp/d03_task3/fail_undefined.o
```

编译阶段没有报错。`nm` 可以看到：

```text
                 U missing_function()
```

这说明目标文件里有对 `missing_function()` 的引用，但没有定义。

### 3.3 链接失败

```bash
g++ /tmp/d03_task3/fail_undefined.o -o /tmp/d03_task3/fail_undefined
```

实际报错：

```text
/usr/bin/ld: /tmp/d03_task3/fail_undefined.o: in function `main':
fail_undefined.cpp:(.text+0x9): undefined reference to `missing_function()'
collect2: error: ld returned 1 exit status
```

### 3.4 为什么编译器当时不知道

编译 `fail_undefined.cpp` 时，编译器只处理这一个 TU：

- 看到了 `int missing_function();`，知道调用形式和返回类型；
- 生成了“调用一个外部符号”的目标代码；
- 它不需要、也无法知道这个符号最终有没有定义；
- 是否所有 `U` 都能被解析，是链接器的工作。

所以 `undefined reference` 不是编译器漏报，而是**链接阶段发现“有人调用，没人实现”**。

常见原因：

- 忘记把实现所在的 `.cpp` / `.o` 加进链接命令；
- 声明和定义的签名不一致（`const`、命名空间、参数类型不同）；
- 定义在某个库里，但库没有链接进来；
- 函数只在头文件里声明， `.cpp` 里根本忘了写。

> **记忆点：** 声明让编译通过，定义让链接通过；`undefined reference` 是“有调用，没有实现”。

## 4. 任务 3.3：复现 `multiple definition`

### 4.1 错误代码

`fail_dup.h`：

```cpp
#pragma once

// 故意失败：普通函数定义放在头文件里。
// 两个 TU 包含它，就会各自生成一份外部链接的 twice。
int twice(int x) {
    return x * 2;
}
```

`fail_dup_a.cpp`：

```cpp
#include "fail_dup.h"

#include <iostream>

int main() {
    std::cout << twice(21) << '\n';
    return 0;
}
```

`fail_dup_b.cpp`：

```cpp
#include "fail_dup.h"

int use_twice(int x) {
    return twice(x) + 1;
}
```

### 4.2 两个 TU 都能单独编译

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -c fail_dup_a.cpp -o /tmp/d03_task3/fail_dup_a.o
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -c fail_dup_b.cpp -o /tmp/d03_task3/fail_dup_b.o
```

两个 `.o` 里都有 `T twice(int)`：

```text
===== fail_dup_a.o =====
0000000000000000 T twice(int)

===== fail_dup_b.o =====
0000000000000000 T twice(int)
0000000000000012 T use_twice(int)
```

### 4.3 链接失败

```bash
g++ /tmp/d03_task3/fail_dup_a.o /tmp/d03_task3/fail_dup_b.o \
    -o /tmp/d03_task3/fail_dup
```

实际报错：

```text
/usr/bin/ld: /tmp/d03_task3/fail_dup_b.o: in function `twice(int)':
fail_dup_b.cpp:(.text+0x0): multiple definition of `twice(int)'; /tmp/d03_task3/fail_dup_a.o:fail_dup_a.cpp:(.text+0x0): first defined here
collect2: error: ld returned 1 exit status
```

### 4.4 为什么 `#pragma once` 没救

`fail_dup.h` 开头有 `#pragma once`，但它的作用范围是**同一个 TU**：

- `fail_dup_a.cpp` 包含一次，展开一次；
- `fail_dup_b.cpp` 也包含一次，展开一次；
- 两个 TU 互不知道对方包含过；
- 于是 `fail_dup_a.o` 和 `fail_dup_b.o` 各有一份 `T twice(int)`；
- 链接器看到两个强定义，报 `multiple definition`。

`#pragma once` 解决的是：

```cpp
// 同一个 .cpp 里
#include "fail_dup.h"
#include "fail_dup.h"   // 第二次不会再展开
```

它解决不了：

```cpp
// a.cpp 和 b.cpp 各自 #include "fail_dup.h"
// 两个 TU 各生成一份定义
```

> **记忆点：** `#pragma once` 只管同一个 TU 不重复包含；跨 TU 的重复定义要靠“只声明”或 `inline`。

### 4.5 修复方式 1：头文件只声明，定义放一个 `.cpp`

`fix_dup_decl.h`：

```cpp
#pragma once

int twice(int x);
```

`fix_dup_def.cpp`：

```cpp
#include "fix_dup_decl.h"

// 整个程序里唯一的 twice 定义。
int twice(int x) {
    return x * 2;
}
```

`fix_dup_main.cpp`：

```cpp
#include "fix_dup_decl.h"

#include <iostream>

int use_twice(int x);

int main() {
    std::cout << "twice(21)=" << twice(21) << '\n';
    std::cout << "use_twice(10)=" << use_twice(10) << '\n';
    return 0;
}
```

`fix_dup_other.cpp`：

```cpp
#include "fix_dup_decl.h"

int use_twice(int x) {
    return twice(x) + 1;
}
```

编译和链接：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -c fix_dup_def.cpp -o /tmp/d03_task3/fix_dup_def.o
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -c fix_dup_main.cpp -o /tmp/d03_task3/fix_dup_main.o
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -c fix_dup_other.cpp -o /tmp/d03_task3/fix_dup_other.o
g++ /tmp/d03_task3/fix_dup_def.o \
    /tmp/d03_task3/fix_dup_main.o \
    /tmp/d03_task3/fix_dup_other.o \
    -o /tmp/d03_task3/fix_dup_decl
/tmp/d03_task3/fix_dup_decl
```

符号关系：

```text
fix_dup_def.o:     T twice(int)
fix_dup_main.o:    U twice(int), U use_twice(int)
fix_dup_other.o:   U twice(int), T use_twice(int)
fix_dup_decl:      T twice(int), T use_twice(int)
```

运行输出：

```text
twice(21)=42
use_twice(10)=21
```

> **记忆点：** 头文件只放声明，定义放一个 `.cpp`，整个程序就只有一个强符号。

### 4.6 修复方式 2：函数放头文件，但加 `inline`

`fix_dup_inline.h`：

```cpp
#pragma once

// 修复方式 2：保留在头文件里，但加上 inline。
inline int twice(int x) {
    return x * 2;
}
```

`fix_dup_inline_main.cpp` / `fix_dup_inline_other.cpp` 和修复 1 的 main/other 基本一样，只是包含这个头文件。

编译和链接：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -c fix_dup_inline_main.cpp -o /tmp/d03_task3/fix_dup_inline_main.o
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -c fix_dup_inline_other.cpp -o /tmp/d03_task3/fix_dup_inline_other.o
g++ /tmp/d03_task3/fix_dup_inline_main.o \
    /tmp/d03_task3/fix_dup_inline_other.o \
    -o /tmp/d03_task3/fix_dup_inline
/tmp/d03_task3/fix_dup_inline
```

两个目标文件里都有 `W twice(int)`：

```text
===== fix_dup_inline_main.o =====
0000000000000000 W twice(int)
                 U use_twice(int)

===== fix_dup_inline_other.o =====
0000000000000000 W twice(int)
0000000000000000 T use_twice(int)
```

链接后的可执行文件只剩一个 `W twice(int)`：

```text
000000000000121b W twice(int)
000000000000122d T use_twice(int)
```

运行输出同样是：

```text
twice(21)=42
use_twice(10)=21
```

`inline` 不只是“建议编译器内联”，它还改变了 ODR 规则：

- 普通函数在多个 TU 中只能有一个定义；
- `inline` 函数允许多个 TU 有相同的定义；
- 编译器把它生成为弱符号 `W`；
- 链接器从多个弱定义中保留一份。

> **记忆点：** 头文件里定义函数要加 `inline`；每个 `.o` 里是弱符号 `W`，链接器合并成一份。

### 4.7 两种修复对比

| 方式 | 头文件内容 | 定义位置 | 符号 |
|---|---|---|---|
| 修复 1 | 只有声明 | 一个 `.cpp` | 该 `.cpp` 里一个 `T` |
| 修复 2 | `inline` 函数定义 | 头文件 | 每个 TU 一个 `W`，链接后一个 |

### 4.8 `#pragma once` 和 ODR 的关系

- `#pragma once` 是**预处理阶段**的机制：防止同一个源文件被重复展开；
- ODR（One Definition Rule）是**语言/链接阶段**的规则：外部链接的实体在整个程序里只能有一个定义；
- 两者解决的问题不同，不能互相替代。

> **记忆点：** `#pragma once` 是预处理层防重复包含，ODR 是链接层单一定义规则，两者不是一回事。

### 4.9 为什么类内成员函数默认是 `inline`

```cpp
class Widget {
public:
    int value() const { return 42; }  // 类内定义，隐式 inline
};
```

类定义通常放在头文件里，被很多 TU 包含。如果类内定义的成员函数不是隐式 `inline`，每个包含头文件的 TU 都会生成一份强定义，必然 `multiple definition`。

所以 C++ 规定：

- 在类定义内部直接定义的成员函数，隐式 `inline`；
- 在类外定义时，如果要放头文件，需要自己写 `inline`；
- 普通自由函数在头文件里定义，也要自己写 `inline`。

> **记忆点：** 类内直接定义的成员函数隐式 `inline`；普通自由函数不会自动 `inline`，放头文件必须自己写。

## 5. 任务 3.4：编译错误 vs 链接错误

| 现象 | 发生在哪个阶段 | 典型原因 | 例子 |
|---|---|---|---|
| `error: expected ',' or ';' before 'return'` | 编译 | 语法错误，缺少分号 | `fail_syntax.cpp` |
| `error: narrowing conversion ... from 'double' to 'int'` | 编译 | 列表初始化禁止窄化 | `fail_narrow.cpp` |
| `undefined reference to 'missing_function()'` | 链接 | 有声明、有调用，没有定义 | `fail_undefined.cpp` |
| `multiple definition of 'twice(int)'` | 链接 | 同一个外部符号在多个 TU 中定义 | `fail_dup.h` + `fail_dup_a/b.cpp` |

### 5.1 编译错误：缺少分号

`fail_syntax.cpp`：

```cpp
int main() {
    int x = 1
    return x;
}
```

实际报错：

```text
fail_syntax.cpp: In function ‘int main()’:
fail_syntax.cpp:4:5: error: expected ‘,’ or ‘;’ before ‘return’
    4 |     return x;
      |     ^~~~~~
fail_syntax.cpp:3:9: error: unused variable ‘x’ [-Werror=unused-variable]
    3 |     int x = 1
      |         ^
cc1plus: all warnings being treated as errors
```

第一个错误是语法问题；第二个是 `-Werror` 把“未使用变量”也升级成了错误。

### 5.2 编译错误：列表初始化窄化

`fail_narrow.cpp`：

```cpp
int main() {
    int x{3.14};
    return x;
}
```

实际报错：

```text
fail_narrow.cpp: In function ‘int main()’:
fail_narrow.cpp:3:11: error: narrowing conversion of ‘3.1400000000000001e+0’ from ‘double’ to ‘int’ [-Wnarrowing]
    3 |     int x{3.14};
      |           ^~~~
```

`{}` 初始化不允许 `double` 到 `int` 这种可能丢失信息的窄化。

### 5.3 链接错误：`undefined reference`

见第 3 节。编译成功，链接失败。

### 5.4 链接错误：`multiple definition`

见第 4 节。两个 TU 都编译成功，链接失败。

> **记忆点：** 编译错误是“这个 TU 自己有问题”，链接错误是“每个 TU 都编译过了，但符号拼不起来”。

## 6. 常见误区

1. **以为头文件里可以直接写普通函数定义**：单个 TU 没问题，多个 TU 包含就会 `multiple definition`；要么只放声明，要么加 `inline`。
2. **以为 `#pragma once` 能解决跨 TU 重复定义**：它只防同一 TU 重复包含。
3. **把 `undefined reference` 当成编译错误**：编译阶段已经过了，是链接器找不到定义。
4. **把 `multiple definition` 当成语法错误**：每个 TU 都编译成功，是链接器看到重复强符号。
5. **在头文件里写 `static` 函数**：每个 TU 各有一份内部链接副本，不会报 multiple definition，但它们不是同一个函数；容易造成行为不一致和代码膨胀。
6. **把 `inline` 只当成优化建议**：`inline` 还涉及 ODR 和链接属性；头文件函数定义必须靠它允许多份相同定义。
7. **声明和定义签名不一致**：比如头文件写 `int f(int)`，实现写 `int f(long)`，链接时仍然找不到 `f(int)`。
8. **`nm` 看到 `U` 就以为一定出错**：`.o` 里的 `U` 是正常的待解析符号；只有最终链接后仍无法解析才是错误。
9. **忘记把实现文件加进链接命令**：只编译了 `main.cpp`，没有链接 `link_stats.o`，就会出现 `undefined reference`。
10. **`-Werror` 只管编译**：它不会让链接错误变成编译错误，也不会替代链接器检查。

> **记忆点：** 看到 `undefined reference` 先找“实现有没有参与链接”；看到 `multiple definition` 先找“头文件里是不是放了定义”。

## 7. 完整复现

在 `code/` 目录下：

```bash
FLAGS="-std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror"
mkdir -p /tmp/d03_task3

# 3.1 正常多文件
g++ $FLAGS -c link_stats.cpp -o /tmp/d03_task3/link_stats.o
g++ $FLAGS -c link_main.cpp -o /tmp/d03_task3/link_main.o
nm -C /tmp/d03_task3/link_main.o | grep -E 'sum|average'
g++ /tmp/d03_task3/link_main.o /tmp/d03_task3/link_stats.o -o /tmp/d03_task3/d03_link
nm -C /tmp/d03_task3/d03_link | grep -E 'sum\(|average\('
/tmp/d03_task3/d03_link

# 3.2 undefined reference
g++ $FLAGS -c fail_undefined.cpp -o /tmp/d03_task3/fail_undefined.o
g++ /tmp/d03_task3/fail_undefined.o -o /tmp/d03_task3/fail_undefined
# 预期：链接报 undefined reference

# 3.3 multiple definition
g++ $FLAGS -c fail_dup_a.cpp -o /tmp/d03_task3/fail_dup_a.o
g++ $FLAGS -c fail_dup_b.cpp -o /tmp/d03_task3/fail_dup_b.o
g++ /tmp/d03_task3/fail_dup_a.o /tmp/d03_task3/fail_dup_b.o -o /tmp/d03_task3/fail_dup
# 预期：链接报 multiple definition

# 3.3 修复 1
g++ $FLAGS -c fix_dup_def.cpp -o /tmp/d03_task3/fix_dup_def.o
g++ $FLAGS -c fix_dup_main.cpp -o /tmp/d03_task3/fix_dup_main.o
g++ $FLAGS -c fix_dup_other.cpp -o /tmp/d03_task3/fix_dup_other.o
g++ /tmp/d03_task3/fix_dup_def.o /tmp/d03_task3/fix_dup_main.o \
    /tmp/d03_task3/fix_dup_other.o -o /tmp/d03_task3/fix_dup_decl
/tmp/d03_task3/fix_dup_decl

# 3.3 修复 2
g++ $FLAGS -c fix_dup_inline_main.cpp -o /tmp/d03_task3/fix_dup_inline_main.o
g++ $FLAGS -c fix_dup_inline_other.cpp -o /tmp/d03_task3/fix_dup_inline_other.o
g++ /tmp/d03_task3/fix_dup_inline_main.o \
    /tmp/d03_task3/fix_dup_inline_other.o -o /tmp/d03_task3/fix_dup_inline
/tmp/d03_task3/fix_dup_inline

# 3.4 编译错误
g++ $FLAGS -c fail_syntax.cpp -o /tmp/d03_task3/fail_syntax.o
g++ $FLAGS -c fail_narrow.cpp -o /tmp/d03_task3/fail_narrow.o
```

## 8. 验收

- [x] `link_stats.cpp`、`link_main.cpp` 能分别编译成 `.o`；
- [x] `link_main.o` 中 `sum`、`average` 是 `U`；
- [x] 链接后的可执行文件中 `sum`、`average` 是 `T`；
- [x] 程序输出 `sum=108`、`average=18`；
- [x] `fail_undefined.cpp` 编译成功、链接报 `undefined reference`；
- [x] `fail_dup_a/b.cpp` 编译成功、链接报 `multiple definition`；
- [x] 修复 1（头文件声明 + 一个 `.cpp` 定义）链接运行成功；
- [x] 修复 2（`inline` 函数放头文件）链接运行成功，目标文件中是 `W`；
- [x] 复现缺少分号和列表初始化窄化的编译错误；
- [ ] 把正常文件加入 CMake target（留给任务 6）。

## 9. 待复习

- 一个 `.cpp` 从预处理到 `.o` 经历了什么？TU 是什么？
- 为什么 `link_main.o` 里 `sum` 是 `U`，而 `link_stats.o` 里是 `T`？
- `undefined reference` 和 `multiple definition` 分别发生在哪个阶段？
- 为什么 `#pragma once` 解决不了跨 TU 的重复定义？
- 头文件里放函数定义的两种正确方式是什么？
- `inline` 除了“建议内联”，还改变了什么规则？
- 为什么类内直接定义的成员函数默认是 `inline`？
- `nm` 的 `U`、`T`、`W` 分别表示什么？
- 如果链接时忘记加 `link_stats.o`，会发生什么？
- 声明和定义的区别是什么？签名不一致会导致什么错误？
