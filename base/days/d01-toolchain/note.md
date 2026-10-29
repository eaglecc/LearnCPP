# D01 - 环境 + 编译模型 + Git

## 今天解决的问题

- 跑通「预处理 → 编译 → 汇编 → 链接」四阶段，能说清每一步的输入、输出和产物格式；
- 分清 `gcc` 与 `g++` 是**驱动程序**而不是编译器本体，知道 C++ 程序链接为什么必须用 `g++`；
- 知道 `-E / -S / -c / 默认` 是同一流水线上的四个「停车点」，以及各自的使用场景；
- 会用 `nm / objdump / file / ldd / readelf` 观察目标文件和可执行文件。

## 核心概念 & 为什么这样设计

### gcc 与 g++：同一个后端，不同的「驱动」

先纠正一个常见误解：**`gcc` 和 `g++` 都不是编译器本体**，它们只是驱动程序（driver），负责按顺序调用真正的工具：

```
g++  -->  cc1plus（C++ 前端：预处理/语法分析/优化/生成汇编）
          |
          +--> as（汇编器：.s -> .o）
          |
          +--> collect2 / ld（链接器：.o + 库 -> 可执行文件）
```

两个驱动调用的是同一套 `cc1plus`、`as`、`ld`，所以生成的目标代码质量完全一样。区别只有两点：

1. **如何识别输入语言**；
2. **链接时自动附加哪些参数**。

| | `gcc` | `g++` |
|---|---|---|
| 按扩展名识别 | `.c` → C，`.cpp` → C++ | 同左 |
| 对 `.c` 文件 | 按 C 编译（标准 `gnu17`） | **按 C++ 编译**（实测符号变成 `_Z1fv`） |
| 对 `.cpp` 文件 | 按 C++ 编译 | 按 C++ 编译 |
| 链接时 | 只加 `-lgcc -lc` | 额外自动加 `-lstdc++ -lm` |
| 编译 C++（`-E/-S/-c`） | 可以 | 可以 |
| 链接 C++ 程序 | **默认失败** | 成功 |

本机实测（GCC 13.3.0）：

```shell
# 编译阶段没问题：gcc 也会调用 C++ 前端
$ gcc -std=c++20 -c hello.cpp -o hello_gcc.o
$ nm hello_gcc.o            # 成功生成，符号和 g++ 生成的一样

# 但链接阶段失败：没有自动链接 libstdc++
$ gcc -std=c++20 hello.cpp -o hello
/usr/bin/ld: hello.cpp:(.text+0x15): undefined reference to `std::cout'
/usr/bin/ld: hello.cpp:(.text+0x24): undefined reference to `std::endl<...>'
collect2: error: ld returned 1 exit status

# 手动把库补上，gcc 也能链接成功
$ gcc -x c++ -std=c++20 hello.cpp -lstdc++ -o hello
$ ./hello
Hello, World!
```

用 `g++ -v` 可以亲眼看到链接命令行里多出来的库：

```shell
$ g++ -std=c++20 -v hello.cpp -o hello_v 2>&1 | tail -2
... /tmp/ccNCZ5Mt.o -lstdc++ -lm -lgcc_s -lgcc -lc -lgcc_s -lgcc ...
```

**结论**：

- 写 C++ 就用 `g++`，不要用 `gcc` 手动补 `-lstdc++`；
- 看到 `undefined reference to std::...` 时，如果确认不是自己少写了定义，先检查是不是链接命令用成了 `gcc`；
- `gcc` 编 `.c` 和 `g++` 编 `.c` 结果不同（后者按 C++ 处理），混编项目里别弄混。

### 编译四阶段

预处理：
```shell
chengyc@ubuntu5815212:~/CPPLearn/base/days/d01-toolchain$ g++ -std=c++20 -E hello.cpp -o hello.i
chengyc@ubuntu5815212:~/CPPLearn/base/days/d01-toolchain$ ll
total 1081
drwxrwxrwx 1 root root       0 Oct 20  2026 ./
drwxrwxrwx 1 root root       0 Oct 18 15:48 ../
-rwxrwxrwx 1 root root      96 Oct 20  2026 hello.cpp*
-rwxrwxrwx 1 root root 1095590 Oct 20  2026 hello.i*
```
此时：
hello.i ： 1095590 字节（43,172 行）
hello.cpp：96 字节


编译：
```shell
chengyc@ubuntu5815212:~/CPPLearn/base/days/d01-toolchain$ g++ -std=c++20 -S hello.i -o hello.s
chengyc@ubuntu5815212:~/CPPLearn/base/days/d01-toolchain$ ll
total 1089
drwxrwxrwx 1 root root       0 Oct 20  2026 ./
drwxrwxrwx 1 root root       0 Oct 18 15:48 ../
-rwxrwxrwx 1 root root      96 Oct 20  2026 hello.cpp*
-rwxrwxrwx 1 root root 1095590 Oct 20  2026 hello.i*
-rwxrwxrwx 1 root root    1554 Oct 20  2026 hello.s*
```
生成的 hello.s 是汇编语言文本，给汇编器 `as` 读，不是直接给机器执行的。

汇编：
```shell
chengyc@ubuntu5815212:~/CPPLearn/base/days/d01-toolchain$ g++ -std=c++20 -c hello.s -o hello.o
chengyc@ubuntu5815212:~/CPPLearn/base/days/d01-toolchain$ ll
total 1093
drwxrwxrwx 1 root root       0 Oct 20 16:17 ./
drwxrwxrwx 1 root root       0 Oct 18 15:48 ../
-rwxrwxrwx 1 root root      96 Oct 20 16:11 hello.cpp*
-rwxrwxrwx 1 root root 1095590 Oct 20 16:12 hello.i*
-rwxrwxrwx 1 root root    2144 Oct 20 16:17 hello.o*
-rwxrwxrwx 1 root root    1554 Oct 20 16:16 hello.s*
```

链接：
```shell
chengyc@ubuntu5815212:~/CPPLearn/base/days/d01-toolchain$ g++ hello.o -o hello
chengyc@ubuntu5815212:~/CPPLearn/base/days/d01-toolchain$ 
total 1117
drwxrwxrwx 1 root root    4096 Oct 20 16:18 ./
drwxrwxrwx 1 root root       0 Oct 18 15:48 ../
-rwxrwxrwx 1 root root   16504 Oct 20 16:18 hello*
-rwxrwxrwx 1 root root      96 Oct 20 16:11 hello.cpp*
-rwxrwxrwx 1 root root 1095590 Oct 20 16:12 hello.i*
-rwxrwxrwx 1 root root    2144 Oct 20 16:17 hello.o*
-rwxrwxrwx 1 root root    1554 Oct 20 16:16 hello.s*
-rwxrwxrwx 1 root root    1381 Oct 20 16:17 note.md*
-rwxrwxrwx 1 root root    7985 Oct 18 15:48 task.md*
chengyc@ubuntu5815212:~/CPPLearn/base/days/d01-toolchain$ ./hello
Hello, World!
```

#### 每一阶段做什么

| 阶段 | 命令 | 黑盒子的名字 | 输入 → 输出 | 做什么 |
|---|---|---|---|---|
| 预处理 | `-E` | `cc1plus -E` | `.cpp` → `.i` | 去注释、展开宏、处理 `#include`/`#if` |
| 编译 | `-S` | `cc1plus` | `.i` → `.s` | 语法/语义分析、优化、生成汇编 |
| 汇编 | `-c` | `as` | `.s` → `.o` | 汇编 → 机器码，生成符号表和重定位表 |
| 链接 | 默认 | `collect2`/`ld` | `.o` + 库 → 可执行 | 合并段、解析符号、重定位、写 ELF |

### `-E` / `-S` / `-c` / 默认：同一流水线上的四个「停车点」

四个开关不是四种不同的编译方式，而是**在流水线的不同位置停下**：

| 开关 | 停在哪 | 产物 | 产物里是什么 | 什么时候用 |
|---|---|---|---|---|
| `-E` | 预处理后 | `.i`（默认打到 stdout） | 展开后的纯 C++ 源码 | 查宏展开、头文件包含关系、条件编译 |
| `-S` | 编译后、汇编前 | `.s` | 汇编文本 | 看优化效果、学汇编、排查代码生成 |
| `-c` | 汇编后、链接前 | `.o` | 可重定位目标文件 | **多文件工程、静态库、构建系统** |
| 无（默认） | 链接后 | 可执行文件 | ELF 可执行 | 单文件小实验、最终产物 |

实测行数（`hello.cpp` 只有 `#include <iostream>` + `main`）：

```shell
$ for o in O0 O1 O2; do g++ -std=c++20 -$o -S hello.cpp -o hello_$o.s; done
$ wc -l hello_O0.s hello_O1.s hello_O2.s
  67 hello_O0.s
  47 hello_O1.s
 102 hello_O2.s
```

**哪个最常用？**

- **日常构建里 `-c` 最常用**：CMake/Make 的本质就是「每个 `.cpp` 独立 `-c` 成 `.o`，变了谁重编谁，最后统一链接」。你看到的构建日志几乎全是 `-c`。
- **手动跑单文件时默认（不带开关）最常见**：`g++ main.cpp -o main`。
- `-S` 用于学习/看优化，`-E` 用于排错（尤其是宏和头文件路径问题），都属于「诊断」用途，不是常规构建步骤。

补充：

- 平时 `g++ -c hello.cpp` 时，中间的 `.s` 是临时文件，用完就删；想看完整四阶段产物加 `-save-temps`。
- `-E` 默认输出到 stdout，记得 `-o`；`-S`/`-c` 默认按输入文件生成同名产物。
- `-c` 可以一次处理多个源文件：`g++ -c a.cpp b.cpp`。

#### 问答：

1. `.i` 文件比源文件大了多少？`#include` 被展开成了什么？

- 96 字节 → 1,095,590 字节，约 **1.1 万倍**；共 **43,172 行**。
- `#include <iostream>` 做的事情就是**把 `/usr/include/c++/13/iostream` 这个文件的全部内容原地粘贴过来**；该头文件自己又 `#include` 了 `bits/requires_hosted.h`、`c++config.h`、`features.h`……层层展开，一共有 **217 个头文件**（`g++ -E -H hello.cpp` 可以看到包含链的树状结构）。
- 粘贴过来的是**声明、inline 函数、模板定义**，不是标准库的机器码。真正的实现（`std::cout`、`operator<<`）在链接阶段来自 `libstdc++.so`。
- 预处理还顺带做了：删除注释、宏展开、处理条件编译。
- `.i` 里仍能看到 1641 行以 `#` 开头的**行标记**（linemarker），形如 `# 3 "hello.cpp" 2`：它告诉编译器和调试器「下面这段代码原本来自哪个文件第几行」，所以 `#include` 指令没了，但 `#` 行还在。
- 验证方法：`.i` 的末尾就是原来的 `main`，前面全是头文件内容。

2. `.s` 是给人看的还是给机器看的？加 `-O0` 和 `-O2` 各生成一次，`main` 的差别是什么？

- `.s` 是**汇编文本**：给汇编器 `as` 读的，机器不直接执行它；人能读懂，属于「人可读的机器码」。
- 行数实测：`-O0` 67 行、`-O1` 47 行、`-O2` **102 行**。注意 **`-O2` 不是最短的，反而最长**（之前笔记里「-O 压缩完 48 行」其实对应 `-O1`）。
- `main` 的差别：
  - `-O0`：保留完整栈帧（`pushq %rbp` / `movq %rsp,%rbp`），参数走寄存器后立刻调 `endl` 的函数指针；
  - `-O1`：去掉 `%rbp` 帧指针，只 `subq $8,%rsp` 做栈对齐，调用序列更干净；
  - `-O2`：**把 `std::endl` 的逻辑内联展开**了——直接 `put('\n')` + `flush`，还带上了 `ctype` 的 widen 判断和异常路径（`__throw_bad_cast`），所以指令更多。
- 结论：**优化不是「行数越少越好」**。`-O2` 用代码体积换运行速度；评价优化要看指令和分支，而不是数行数。

3. `.o` 里有什么？用 `nm -C hello.o` 看符号，哪些是 `T`、哪些是 `U`？

```
                 U _GLOBAL_OFFSET_TABLE_        # GOT 基址，PIC 寻址用
0000000000000000 T main                          # 已定义，在 .text
                 U std::ostream::operator<<(std::ostream& (*)(std::ostream&))
000000000000000e r std::__detail::__integer_to_chars_is_unsigned<unsigned int>
                 U std::ios_base_library_init()
                 U std::cout
                 U std::endl<...>
                 U std::operator<< <...>(..., char const*)
```

- `T`（大写）= `.text` 段中**已定义**的全局函数。本 TU 里只有 `main` 是自己定义的。
- `U`（大写）= **undefined**，只是「引用/声明」，实现不在这个 `.o` 里，需要链接器去别的 `.o` 或 `libstdc++.so` 里找。
- `r`（小写）= 只读数据段（`.rodata`）里的**局部**符号，这里是模板实例化出来的常量。
- `W`（大写）= **weak（弱符号）**：允许在多个 TU 里各有一份定义，链接器任选一份、其余丢弃。`inline` 函数、模板实例化、类内定义的成员函数都会产生 `W`（见下面 ODR 实验）。
- 小写字母表示局部符号（local），大写表示全局符号（global）；`U` 表示未定义。
- `.o` 里除了符号，还有：机器码（`.text`）、只读数据（`.rodata`）、**重定位表**（哪些位置需要链接器填地址）、符号表、调试信息。
- 链接后验证：`std::cout` 会变成可执行文件里的 `B`（BSS，拷贝重定位），而 `operator<<` 仍是 `U`，带 `@GLIBCXX_3.4` 版本标记，由动态链接器在**运行时**解析：

```shell
$ nm -C hello | grep -E 'std::cout|operator<<'
0000000000004040 B std::cout@GLIBCXX_3.4
                 U std::ostream::operator<<(std::ostream& (*)(std::ostream&))@GLIBCXX_3.4
```

4. `file hello.o` 和 `file hello` 分别输出什么？

```shell
$ file hello.o
hello.o: ELF 64-bit LSB relocatable, x86-64, version 1 (SYSV), not stripped
$ file hello
hello: ELF 64-bit LSB pie executable, x86-64, version 1 (SYSV), dynamically linked, interpreter /lib64/ld-linux-x86-64.so.2, BuildID[sha1]=..., for GNU/Linux 3.2.0, not stripped
```

- **relocatable（可重定位）**：地址还没定，各段都从 0 开始算，留着重定位表等链接器合并。
- **pie executable（位置无关可执行文件）**：地址已定，但整体可以被加载到任意地址（PIE），增加了安全性。
- **interpreter**：程序启动时先运行 `/lib64/ld-linux-x86-64.so.2`，由它加载依赖库。
- **dynamically linked**：依赖共享库，不是静态链接。
- **not stripped**：符号表还在（可以用 `nm`）。发布时可 `strip` 去掉。

5. `ldd ./hello` 显示了什么？程序运行时依赖谁？

```shell
$ ldd ./hello
        linux-vdso.so.1 (0x000074396de65000)
        libstdc++.so.6 => /lib/x86_64-linux-gnu/libstdc++.so.6 (...)
        libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (...)
        libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (...)
        /lib64/ld-linux-x86-64.so.2 (...)
        libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (...)
```

逐一说明：

| 库 | 作用 |
|---|---|
| `linux-vdso.so.1` | 内核映射到用户态的虚拟库（`gettimeofday` 等），不是磁盘文件 |
| `libstdc++.so.6` | C++ 标准库实现（`std::cout`、`operator<<` 等） |
| `libc.so.6` | C 标准库 / 系统调用封装 |
| `libm.so.6` | 数学库，被其他库依赖 |
| `libgcc_s.so.1` | GCC 运行时（异常展开、部分底层运算） |
| `ld-linux-x86-64.so.2` | 动态链接器**本身**，负责启动时加载以上所有库并做重定位 |

- 运行时的真正入口是动态链接器：它读取可执行文件的 `DT_NEEDED` 项（`readelf -d hello | grep NEEDED`）来加载依赖。
- `ldd` 只是「模拟」显示依赖，不是真的运行程序。缺库时会报 `error while loading shared libraries`。
- 依赖是基于**文件名/SONAME**找的，所以部署时版本不匹配会出现 `GLIBCXX_3.4.xx not found`。

6. `objdump -d hello.o | head -50` 能看懂哪几条指令？

```asm
0000000000000000 <main>:
   0:   f3 0f 1e fa             endbr64
   4:   55                      push   %rbp
   5:   48 89 e5                mov    %rsp,%rbp
   8:   48 8d 05 00 00 00 00    lea    0x0(%rip),%rax
   f:   48 89 c6                mov    %rax,%rsi
  12:   48 8d 05 00 00 00 00    lea    0x0(%rip),%rax
  19:   48 89 c7                mov    %rax,%rdi
  1c:   e8 00 00 00 00          call   21 <main+0x21>
  21:   48 8b 15 00 00 00 00    mov    0x0(%rip),%rdx
  28:   48 89 d6                mov    %rdx,%rsi
  2b:   48 89 c7                mov    %rax,%rdi
  2e:   e8 00 00 00 00          call   33 <main+0x33>
  33:   b8 00 00 00 00          mov    $0x0,%eax
  38:   5d                      pop    %rbp
  39:   c3                      ret
```

逐条：

| 指令 | 含义 |
|---|---|
| `endbr64` | CET 安全特性：标记「间接跳转的合法落点」 |
| `push %rbp` / `mov %rsp,%rbp` | 建立栈帧（`-O0` 特有，`-O1` 起就没有了） |
| `lea 0x0(%rip),%rax` | 取 RIP 相对地址。**这里偏移是 0，是因为 `.o` 还没重定位**，链接后会被填成字符串和 `std::cout` 的真实地址 |
| `mov %rax,%rsi` / `%rdi` | 按 System V AMD64 ABI 传参：`rdi` 是第 1 个参数（`std::cout`），`rsi` 是第 2 个（字符串） |
| `call 21 <main+0x21>` | **目标地址是 0，且指向下一条指令**——占位符，链接器会把它改成 `operator<<` 的真实地址 |
| `mov 0x0(%rip),%rdx` | 从 GOT 取 `std::endl` 的函数指针 |
| `mov $0x0,%eax` | `return 0`：返回值放 `eax` |
| `pop %rbp` / `ret` | 恢复栈帧并返回 |

关键观察：**`.o` 里所有跨文件调用/地址都是 0**，这就是「可重定位」的含义；链接器的工作就是把这些洞填上。反汇编时看到 `call` 后面是 0，不要以为是 bug。

### CMake 与构建目录

## 易错点 / UB（附最小复现代码）

### 易错点 1：头文件里定义普通函数 → multiple definition

#### 复现代码

`odr_shared.h`（**定义**，不是声明）：

```cpp
// odr_shared.h
int add(int a, int b) {
    return a + b;
}
```

```cpp
// odr_a.cpp
#include "odr_shared.h"
int main() {
    int result = add(3, 4);
}
```

```cpp
// odr_b.cpp
#include "odr_shared.h"
int use_add() {
    return add(1, 2);
}
```

#### 第一步：分别编译——都不报错

```shell
$ g++ -std=c++20 -c odr_a.cpp -o ~/build/d01-odr/odr_a.o
$ g++ -std=c++20 -c odr_b.cpp -o ~/build/d01-odr/odr_b.o
```

**这是最容易误判的地方**：每个 `.cpp` 是一个独立的编译单元（TU），编译器编译 `odr_a.cpp` 时看不到 `odr_b.cpp`，所以两个 TU 都「合法」。

用 `nm` 看，两个 `.o` 里都有同一个 **`T`**：

```shell
$ nm -C ~/build/d01-odr/odr_a.o | grep add
0000000000000000 T add(int, int)
$ nm -C ~/build/d01-odr/odr_b.o | grep add
0000000000000000 T add(int, int)
```

#### 第二步：链接——报错

```shell
$ g++ ~/build/d01-odr/odr_a.o ~/build/d01-odr/odr_b.o -o ~/build/d01-odr/odr
/usr/bin/ld: /tmp/odr_b.o: in function `add(int, int)':
odr_b.cpp:(.text+0x0): multiple definition of `add(int, int)';
 /tmp/odr_a.o:odr_a.cpp:(.text+0x0): first defined here
collect2: error: ld returned 1 exit status
```

链接器要合并所有 `.o`，发现两个**同名的强符号**（`T`），只能报错。

#### 修复方式一：头文件只留声明，定义放某一个 `.cpp`

```cpp
// odr_shared.h
#pragma once
int add(int a, int b);          // 声明：只告诉编译器「有这个函数」
```

```cpp
// odr_a.cpp
#include "odr_shared.h"
int add(int a, int b) { return a + b; }   // 全程序唯一的定义
int main() { return add(3, 4); }
```

修复后符号变化：

| 文件 | 修复前 | 修复后 |
|---|---|---|
| `odr_a.o` | `T add` | `T add`（定义在这里） |
| `odr_b.o` | `T add` | `U add`（引用，等链接器解析） |

一个 `T` 配若干 `U`，链接器把 `U` 全部指向这个 `T`，成功。

#### 修复方式二：写成 `inline`，保留在头文件

```cpp
// odr_shared.h
#pragma once
inline int add(int a, int b) { return a + b; }
```

符号类型从 `T` 变成 **`W`（weak）**：

```shell
$ nm -C ~/build/d01-odr/odr_b.o | grep add
0000000000000000 W add(int, int)
```

- 强符号（`T`）全程序只能有一个；**弱符号（`W`）允许多个**。
- 链接器遇到多个 `W` 时任选一份，其余丢弃。
- 前提：每份定义必须**完全一致**（这正是 ODR 的要求）。

`inline` 在 C++ 里更重要的含义不是「建议内联」，而是**允许这个函数在多个 TU 中重复定义**。类内定义的成员函数默认就是 `inline`，所以头文件里写完整的类通常不会触发 multiple definition。

#### 为什么 `#pragma once` / include guard 救不了？

给 `odr_shared.h` 加上 `#pragma once` 后重新编译链接，**仍然是同样的 `multiple definition`**。

原因：

- `#pragma once` 是**单个 TU 内**的去重：防止同一个头文件在一次预处理中被重复粘贴。
- ODR 冲突发生在**两个 TU 之间**：`odr_a.cpp` 和 `odr_b.cpp` 各自预处理时，都会把 `add` 的定义粘进自己的那份代码。每个 TU 只包含了一次，guard 根本没有机会生效。
- 链接器看到的是两份独立的 `.o`，各自带着 `T add`；它不知道也不关心这两个定义来自同一个头文件。

一句话：**include guard 解决「同一份代码被粘两次」；单一定义或 `inline` 解决「全程序到处都有一份定义」。**

#### 结论速查

| 现象 | 发生在哪个阶段 | 常见原因 |
|---|---|---|
| `undefined reference to ...` | 链接 | 只有 `U` 没有 `T`：少定义、少链接 `.o`/库、库顺序不对 |
| `multiple definition of ...` | 链接 | 多个 `.o` 都有同一个 `T`：头文件里放了非 `inline` 的定义 |
| 编译通过但链接失败 | — | 编译器按 TU 工作，看不到别的 TU；跨 TU 的问题只有链接器能发现 |

## 代码片段

## 待复习
