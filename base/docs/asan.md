# ASan 笔记：AddressSanitizer 是什么、怎么用、怎么读报错

> 环境：WSL2 Ubuntu 24.04，g++ 13.3.0。
> 本文所有报错都来自实际编译、运行；示例源码放在 `/tmp`，不影响仓库构建。

## 0. 速查

- **ASan = AddressSanitizer**：编译器插桩 + 运行时库，检测内存安全错误。
- 启用：`-fsanitize=address -g -fno-omit-frame-pointer`，可叠加 `-fsanitize=address,undefined`。
- 常抓：堆 / 栈 / 全局缓冲区越界、use-after-free、use-after-return、double-free、`alloc-dealloc-mismatch`、内存泄漏。
- 抓不到：未初始化读取、数据竞争、逻辑错误、fd / socket 泄漏。
- 报告三段式：**出错点调用栈 + 释放点 + 分配点**，最后一行 `SUMMARY`。
- 程序 `abort` 时 stdout 可能还没刷出来，日志用 `std::endl` 或 `std::flush`。

## 1. ASan 是什么

ASan 是 GCC / Clang 提供的内存错误检测工具。它不是独立的“检查程序”，而是分两部分：

1. **编译期插桩**：编译器在每次内存读写、每次 `new` / `delete` 前后插入检查代码；
2. **运行期库**：接管内存分配器，给每块内存加“护栏”，维护影子内存（shadow memory）。

因此启用 ASan 必须**重新编译**，不能用它检查一个已经编译好的普通二进制。

它属于 sanitizer 家族：

| 工具 | 抓什么 | 是否随 ASan 一起 |
|---|---|---|
| **ASan** | 越界、use-after-free、double-free、分配器不匹配 | 主体 |
| **LeakSanitizer (LSan)** | 堆内存泄漏 | Linux 上默认包含 |
| **UBSan** | 有符号溢出、空指针、越界移位等 UB | 需另外加 `-fsanitize=undefined` |
| **TSan** | 数据竞争 | 不能和 ASan 同时启用 |
| **MSan** | 未初始化读取 | 需单独启用，主要用 Clang |

和 Valgrind 的区别：Valgrind 不需要重新编译，但慢得多（常见 10–50 倍）；ASan 要重新编译，通常慢约 2 倍，更适合放进日常测试和 CI。

> **记忆点：** ASan 是“编译期插桩 + 运行期 shadow memory”，必须带 flag 重新编译；它不是 Valgrind 那种外部动态翻译工具。

## 2. 怎么用

### 2.1 最小命令

```bash
g++ -std=c++20 -g -fsanitize=address -fno-omit-frame-pointer \
    main.cpp -o main_asan
./main_asan
```

| flag | 作用 |
|---|---|
| `-fsanitize=address` | 插入 ASan 检查并链接运行时库 |
| `-g` | 保留调试信息，报告里才有文件名和行号 |
| `-fno-omit-frame-pointer` | 保留帧指针，调用栈才完整 |
| `-O0` / `-O1` | 调试时常用；优化可以保留，但报错行号可能受内联影响 |

### 2.2 和 UBSan 一起用

```bash
g++ -std=c++20 -g -fsanitize=address,undefined \
    -fno-omit-frame-pointer main.cpp -o main_asan
```

如果希望 UBSan 报告后立即终止：

```bash
-fsanitize=undefined -fno-sanitize-recover=all
```

### 2.3 CMake 里开关

```cmake
option(ENABLE_SANITIZERS "Build with ASan + UBSan" OFF)

if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer -g)
    add_link_options(-fsanitize=address,undefined)
endif()
```

```bash
cmake -S . -B build -DENABLE_SANITIZERS=ON
cmake --build build
./build/demo
```

这个片段实际构建过：带越界写的 `demo` 运行时正确报出 `heap-buffer-overflow`。

### 2.4 常用 `ASAN_OPTIONS`

```bash
ASAN_OPTIONS=detect_leaks=1:halt_on_error=1 ./main_asan
```

| 选项 | 用途 |
|---|---|
| `detect_leaks=1` | 开启泄漏检测（Linux 默认开） |
| `halt_on_error=1` | 第一次报错就终止（默认） |
| `detect_stack_use_after_return=1` | 加强返回后使用栈对象的检测 |
| `alloc_dealloc_mismatch=1` | `new[]` / `delete` 混用检测（默认开） |
| `log_path=asan.log` | 报告写入文件而不是 stderr |
| `exitcode=42` | 自定义报错后的退出码 |

> **记忆点：** `-fsanitize=address -g -fno-omit-frame-pointer` 是最小三件套；所有参与链接的源文件都要用同一套 sanitizer flag 重新编译。

## 3. 原理（够用版）

1. 进程地址空间里留出约 **1/8** 的 **shadow memory**，每个字节记录对应 8 字节应用内存的状态；
2. 每次分配内存，在前后加 **redzone**（红区），并标记可用 / 已释放 / 红区；
3. 每次读写前查 shadow memory，命中红区或已释放区域就报错；
4. 释放时不立即归还，而是放进 **quarantine**（隔离区），这样“释放后使用”能被抓到；
5. 分配器记录每次分配的调用栈，所以报告能同时给出“在哪分配、在哪释放、在哪再次访问”。

Quarantine 的代价是内存占用变高，但换来的是 use-after-free 的检出率。

> **记忆点：** shadow memory 负责“这块地址现在能不能碰”，redzone 负责“边界”，quarantine 负责“释放后还能不能碰”。

## 4. 报错怎么读

一个完整的堆越界报告（地址、BuildId 随运行变化，部分重复帧略）：

```text
=================================================================
==1581==ERROR: AddressSanitizer: heap-buffer-overflow on address 0x502000000020 at pc 0x5c7a37e862eb bp 0x7ffced521500 sp 0x7ffced5214f0
WRITE of size 4 at 0x502000000020 thread T0
    #0 0x5c7a37e862ea in main /tmp/asan_cmake/main.cpp:3
    #1 0x7bf4c7c2a1c9 in __libc_start_call_main ../sysdeps/nptl/libc_start_call_main.h:58
    #2 0x7bf4c7c2a28a in __libc_start_main_impl ../csu/libc-start.c:360
    #3 0x5c7a37e86184 in _start (/tmp/asan_cmake/build/demo+0x1184)

0x502000000020 is located 0 bytes after 16-byte region [0x502000000010,0x502000000020)
allocated by thread T0 here:
    #0 0x7bf4c8cfe6c8 in operator new[](unsigned long) .../asan_new_delete.cpp:98
    #1 0x5c7a37e8625f in main /tmp/asan_cmake/main.cpp:2
    #2 0x7bf4c7c2a1c9 in __libc_start_call_main ...

SUMMARY: AddressSanitizer: heap-buffer-overflow /tmp/asan_cmake/main.cpp:3 in main
Shadow bytes around the buggy address:
  0x501fffffff80: 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00 00
=>0x502000000000: fa fa 00 00[fa]fa fa fa fa fa fa fa fa fa fa fa
  0x502000000080: fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa fa

Shadow byte legend (one shadow byte represents 8 application bytes):
  Addressable:           00
  Heap left redzone:       fa
  Freed heap region:       fd
  Stack after return:      f5
```

逐段解释：

| 部分 | 含义 |
|---|---|
| `ERROR: AddressSanitizer: heap-buffer-overflow` | 错误类型 |
| `on address ... at pc ... bp ... sp ...` | 出错地址、程序计数器、栈帧指针 |
| `WRITE of size 4` | 写 4 字节；读越界会写 `READ of size N` |
| `#0 ... main main.cpp:3` | 出错点调用栈，`#0` 是最内层 |
| `located 0 bytes after 16-byte region` | 越界位置：16 字节块的紧后面 |
| `allocated by thread T0 here:` | 这块内存的分配调用栈 |
| `SUMMARY` | 一行结论，CI 里常直接 grep 这一行 |
| shadow bytes | 每个影子字节代表 8 字节应用内存；`fa` = 堆红区，`fd` = 已释放，`00` = 可访问 |

读报告的顺序建议：**先看 `SUMMARY` 的源码行 → 再看 `WRITE/READ` 和 `located ...` → 最后看 `allocated by` 确认对象是谁分配的**。

> **记忆点：** ASan 报告的核心是“访问点 + 分配点（+ 释放点）”三处调用栈，`SUMMARY` 是给 CI 用的单行结论。

## 5. 常见错误实测

### 5.1 堆越界写：heap-buffer-overflow

```cpp
int* p = new int[4];
p[4] = 1;   // 越界：合法下标是 0..3
delete[] p;
```

```text
ERROR: AddressSanitizer: heap-buffer-overflow on address 0x502000000020
WRITE of size 4 at 0x502000000020 thread T0
0x502000000020 is located 0 bytes after 16-byte region [0x502000000010,0x502000000020)
allocated by thread T0 here:
    #1 ... in main /tmp/asan_cmake/main.cpp:2
SUMMARY: AddressSanitizer: heap-buffer-overflow /tmp/asan_cmake/main.cpp:3 in main
```

**原因**：`new int[4]` 只分配 16 字节，`p[4]` 写到第 17–20 字节，落在 redzone 里。

> **记忆点：** 堆越界报告会给出“距合法区域多少字节”和分配点；多写 1 字节也能定位。

### 5.2 栈越界写：stack-buffer-overflow

```cpp
int a[3] = {1, 2, 3};
a[3] = 42;   // 越界写栈数组
```

```text
ERROR: AddressSanitizer: stack-buffer-overflow on address 0x731e95c0002c
WRITE of size 4 at 0x731e95c0002c thread T0
Address 0x731e95c0002c is located in stack of thread T0 at offset 44 in frame
SUMMARY: AddressSanitizer: stack-buffer-overflow /tmp/stomp_stack.cpp:3 in main
```

栈上的越界会破坏相邻局部变量，没有 ASan 时可能表现为“某个变量莫名其妙变了”，也可能完全不崩。

> **记忆点：** 栈越界的报告会说明“在第几个栈帧、偏移多少字节”，比调试器更直接。

### 5.3 释放后使用：heap-use-after-free

```cpp
int* p = new int(42);
delete p;
std::cout << *p << '\n';   // 读已释放内存
```

```text
ERROR: AddressSanitizer: heap-use-after-free on address 0x502000000010
READ of size 4 at 0x502000000010 thread T0
freed by thread T0 here:
    #1 ... in main /tmp/asan_uaf.cpp:4
previously allocated by thread T0 here:
    #1 ... in main /tmp/asan_uaf.cpp:3
SUMMARY: AddressSanitizer: heap-use-after-free /tmp/asan_uaf.cpp:5 in main
```

报告里同时有 `freed by`（第 4 行 `delete p`）和 `previously allocated by`（第 3 行 `new`），一看就知道生命周期哪里断了。

> **记忆点：** use-after-free 报告一定会带 `freed by`，这是定位“谁先释放了它”的关键。

### 5.4 返回后使用栈对象：stack-use-after-return

```cpp
int* g = nullptr;

void escape() {
    int local = 7;
    g = &local;   // 把局部变量地址带出去
}

int main() {
    escape();
    std::printf("%d\n", *g);   // 访问已返回函数的栈对象
}
```

```text
ERROR: AddressSanitizer: stack-use-after-return on address 0x7063b0000020
READ of size 4 at 0x7063b0000020 thread T0
Address 0x7063b0000020 is located in stack of thread T0 at offset 32 in frame
SUMMARY: AddressSanitizer: stack-use-after-return /tmp/asan_suar.cpp:9 in main
```

某些环境默认不检测这一类错误，可以显式打开：

```bash
ASAN_OPTIONS=detect_stack_use_after_return=1 ./main_asan
```

> **记忆点：** 返回局部变量地址是悬垂指针，不是泄漏；ASan 用 `stack-use-after-return` 抓它，必要时打开 `detect_stack_use_after_return`。

### 5.5 重复释放：double-free

任务 2 的 `bad_string.cpp`：

```cpp
BadString a("hello");
BadString b = a;   // 浅拷贝：两个对象的 data_ 指向同一块内存
```

```text
ERROR: AddressSanitizer: attempting double-free on 0x502000000010 in thread T0
    #1 ... in BadString::~BadString() .../bad_string.cpp:14
freed by thread T0 here:
    #1 ... in BadString::~BadString() .../bad_string.cpp:14
previously allocated by thread T0 here:
    #1 ... in BadString::BadString(char const*) .../bad_string.cpp:10
SUMMARY: AddressSanitizer: double-free ... in operator delete[](void*)
```

两份析构调用栈指向同一行，说明同一个对象被释放了两次；根因是浅拷贝让两个对象共享一块内存。

> **记忆点：** double-free 的两条析构栈通常一模一样，看到它就回头查“谁和谁共享了同一块资源”。

### 5.6 分配 / 释放方式不匹配：alloc-dealloc-mismatch

```cpp
int* p = new int[4]{1, 2, 3, 4};
delete p;   // 错误：new[] 应配 delete[]
```

```text
ERROR: AddressSanitizer: alloc-dealloc-mismatch (operator new [] vs operator delete) on 0x502000000010
    #0 ... in operator delete(void*, unsigned long)
allocated by thread T0 here:
    #0 ... in operator new[](unsigned long)
SUMMARY: AddressSanitizer: alloc-dealloc-mismatch ... in operator delete(void*, unsigned long)
```

对 `int` 这种平凡类型，混用可能“看起来能跑”，但仍然是 UB；对带析构函数的元素，`delete[]` 才会逐个调用析构。

> **记忆点：** `new[]` / `delete[]` 必须成对；ASan 用 `alloc-dealloc-mismatch` 抓这种“看起来没崩”的 UB。

### 5.7 内存泄漏：LeakSanitizer

```cpp
int main() {
    int* p = new int[10];
    p[0] = 1;
    (void)p;   // 故意不 delete[]
}
```

```text
ERROR: LeakSanitizer: detected memory leaks
Direct leak of 40 byte(s) in 1 object(s) allocated from:
    #1 0x63806c2921be in main /tmp/asan_leak.cpp:2
SUMMARY: AddressSanitizer: 40 byte(s) leaked in 1 allocation(s).
```

注意前缀是 `LeakSanitizer` 而不是 `AddressSanitizer`：它挂在 ASan 运行时里，在程序正常退出时检查。本次运行退出码为 1。

> **记忆点：** 泄漏报告在**正常退出**时才产生；程序中途 `abort` 或调用 `_exit` 时，LeakSanitizer 可能来不及检查。

## 6. 它抓不到什么

| 问题 | ASan 能抓吗 | 该用什么 |
|---|---|---|
| 未初始化读取 | ❌ | MSan / Valgrind |
| 数据竞争 | ❌ | TSan |
| 有符号整数溢出、非法移位 | ❌ | UBSan |
| 文件描述符 / socket 泄漏 | ❌ | `/proc/self/fd`、`lsof` |
| 逻辑错误（算错、写错分支） | ❌ | 测试、code review |
| 没执行到的代码路径 | ❌ | 提高覆盖率、fuzz |
| 自定义分配器绕过 `new` / `delete` | 可能漏 | 配合 Valgrind、专门测试 |

特别强调：**ASan 只跟踪堆 / 栈 / 全局内存，不跟踪文件描述符**。任务 4 里 `fopen` 5 次不 `fclose`，ASan 不会有任何报告，只能用 `/proc/self/fd` 数 fd（实测从 7 涨到 12）。

> **记忆点：** ASan 是内存检查器，不是“万能检查器”；fd 泄漏、数据竞争、未初始化值分别归 `/proc`、TSan、MSan 管。

## 7. 和其他工具的分工

| 工具 | 检测目标 | 需要重新编译 | 典型开销 |
|---|---|---|---|
| ASan | 内存越界、use-after-free、double-free | 是 | 约 2 倍 |
| LSan（含在 ASan 中） | 堆泄漏 | 是 | 小 |
| UBSan | 语言层 UB：溢出、空指针、非法转换 | 是 | 小 |
| TSan | 多线程数据竞争 | 是 | 约 5–15 倍 |
| MSan | 未初始化读取 | 是（主要 Clang） | 约 3 倍 |
| Valgrind Memcheck | 内存错误 + 泄漏 | 否 | 10–50 倍 |

组合规则：

- `-fsanitize=address,undefined` ✅ 常用组合；
- ASan + TSan ❌ 不能同时启用；
- MSan 通常单独使用，因为它要求所有代码（包括库）都用 MSan 编译。

> **记忆点：** 日常用 ASan + UBSan；并发问题换 TSan；未初始化读换 MSan / Valgrind；它们不能互相替代。

## 8. 成本与使用建议

- 开销：通常慢约 2 倍、内存约 2–3 倍；quarantine 会额外占内存，可以调小：
  ```bash
  ASAN_OPTIONS=quarantine_size_mb=64 ./main_asan
  ```
- 用在**调试构建和测试**里，不要带进 release 产物；
- 保持 `-g`，否则报告只有地址没有行号；
- 对关键路径、解析器、字符串 / 容器操作优先跑 ASan；
- 把踩过的坏例子保留成回归测试，修复后每次 CI 都跑一遍；
- 程序在 ASan 第一次报错处就 `abort`，**后面的错误和泄漏可能都看不到**；修完一个再跑下一个。

> **记忆点：** ASan 适合“每次提交都跑”，不适合发布包；它的价值在于把延迟崩溃变成第一次出错就带现场报告。

## 9. 易错点

1. **忘记重新编译**：只给 `main.o` 加 flag，其他 TU 没加，检测不完整甚至链接失败。
2. **忘记 `-g`**：报告只有地址和 `??`，定位困难。
3. **忘记 `-fno-omit-frame-pointer`**：调用栈缺帧，尤其是优化构建。
4. **以为 ASan 能抓所有问题**：未初始化读、数据竞争、fd 泄漏它都不管。
5. **程序 `abort` 后看不到之前的 `std::cout`**：stdout 块缓冲未刷新；日志改用 `std::endl` / `std::flush`。
6. **第一次报错就 abort，后面的泄漏没报**：别把“只报了一个错”当成“只有一个错”。
7. **修复后不重跑**：修复可能引入新的越界 / 泄漏，ASan 要作为回归检查常驻。
8. **ASan 和 TSan 一起开**：不支持，编译或运行会失败。
9. **LeakSanitizer 只报堆内存**：`fopen` 不关、`std::thread` 不 join 它都看不到。
10. **认为“普通构建能跑”就等于正确**：越界和 use-after-free 常常只在特定堆布局下崩溃。

## 10. 待复习

- ASan 的编译期和运行期各做什么？为什么必须重新编译？
- `-fsanitize=address`、`-g`、`-fno-omit-frame-pointer` 各自解决什么问题？
- shadow memory、redzone、quarantine 分别起什么作用？
- 一份 ASan 报告里，“访问点 / 分配点 / 释放点”分别在哪里看？
- `heap-buffer-overflow`、`stack-buffer-overflow`、`heap-use-after-free`、`double-free`、`alloc-dealloc-mismatch` 分别对应什么错误？
- LeakSanitizer 的报告前缀是什么？为什么程序 abort 时可能看不到泄漏？
- ASan 抓不到哪些问题？分别该用哪个工具？
- 为什么日志在 ASan 报错前可能丢失？怎么避免？
