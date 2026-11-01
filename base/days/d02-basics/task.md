# D02 任务：基础类型 + Linux 基础

> 预计 4.5–5 小时。概念 1h / 动手 2.5h / 笔记 1h。
> 今天一半是 C++ 的类型系统地基，一半是 Linux 日常操作与进程/信号。两边都要动手，不要只看。

## 今日目标

- [x] 说清基本类型的大小、对齐、初始化方式与窄化
- [x] 分清 `const` / `constexpr` / `auto`，能读懂 `const` 与指针组合
- [ ] 分清引用与指针，知道各自适用场景和常见悬空问题
- [ ] 对左值/右值有直觉（今天只要求“认识”，不要求完美转发）
- [ ] 掌握文件权限、进程、信号，能用 `strace` 观察程序行为
- [ ] 完成练习集 + `docs/linux-basics.md` + `note.md` + Git 提交

## 今日不碰

类、动态内存（`new/delete`、智能指针）、模板、异常、多线程。`std::move` 只认识名字，不展开。

---

## 任务 1：基本类型与初始化（约 60 min）

在本目录写 `types.cpp`，逐项动手：

1. **固定宽度类型**：`<cstdint>` 的 `int8_t`…`uint64_t`，以及 `std::size_t`、`std::ptrdiff_t`。各打印一次 `sizeof` 和 `std::numeric_limits<T>::min()/max()`。
2. **大小与对齐表**：打印 `bool / char / short / int / long / long long / float / double / long double / 指针 / size_t` 的 `sizeof` 与 `alignof`。记录 x86-64 Linux 上的结果，并和 Windows（若手头能编译）对比 `long`、指针、`long double` 的差异。
3. **四种初始化**：
   - `int a;`（默认初始化，函数内是未定义值）
   - `int b = 1;`（拷贝初始化）
   - `int c(1);`（直接初始化）
   - `int d{1};`（列表初始化）
4. **窄化**：`int x{3.14};`、`char c{1000};` 必须**编译失败**。记录报错原文。再用 `-Wconversion` 观察 `double → int` 的隐式转换警告。
5. **符号与溢出**：
   - 有符号溢出是 UB，无符号溢出按模回绕；
   - 写一个带 `#ifdef UB_DEMO` 的段：`int x = INT_MAX; x += 1;`，用 `-fsanitize=undefined -fno-sanitize-recover=all` 编译运行，观察 UBSan 报错；
   - 注意：**不要**把 UB 当成“结果可预测”。记录 UBSan 的输出。
6. **字符与字符串字面量**：C++20 中字符串字面量是 `const char[N]`，`char* s = "abc"` 编译失败或至少不合法；解释为什么。

> 编译要求：`g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion`，警告一条条处理，不要用 `(int)` 硬压，先想清楚转换是否必要。

## 任务 2：const / constexpr / auto（约 30 min）

在 `types.cpp` 中继续：

1. **const 与指针**，写代码并注释每个声明的含义（从右往左读）：
   
   ```cpp
   int value = 1;
   const int* p1 = &value;        // ?
   int* const p2 = &value;        // ?
   const int* const p3 = &value;  // ?
   ```
   
   分别尝试通过它们修改值 / 修改指针指向，把编译不过的行注释掉并写明原因。

2. **constexpr**：写 `constexpr int square(int x) { return x * x; }`，用 `static_assert(square(5) == 25)` 验证编译期求值；再写一个**不能**是 `constexpr` 的函数（例如含 `std::cout`），观察报错。

3. **auto 的坑**：
   
   - `const int ci = 1; auto a = ci;` —— `a` 是 `const int` 吗？
   - `auto& r = ci;` 与 `auto r2 = ci;` 的区别；
   - `auto` 对数组退化的影响：`int arr[5]; auto p = arr;` 的 `p` 类型是什么？
   - 用 `typeid(...).name()` 或 `static_assert(std::is_same_v<...>)` 验证你的判断。

## 任务 3：引用与指针（约 45 min）

新建 `pointers.cpp`，完成一组小练习（每个函数写指针版和引用版，能对比就对比）：

1. 交换两个 `int`：`swap_ptr(int*, int*)` 与 `swap_ref(int&, int&)`；
2. 求数组最大值：用指针遍历；再写一个 `int max_of(const int* arr, size_t n)`；
3. 数组反转（原地）；
4. 统计数组中某个值出现次数；
5. **const 正确性**：`process(const std::vector<int>& v)` vs `process(std::vector<int> v)`，各调用一次，观察拷贝；
6. **数组退化**：在 `main` 里 `sizeof(arr)` 和在函数参数里 `sizeof(arr)` 结果为什么不同？写代码验证；
7. **悬空指针**：写一个返回局部变量地址的函数，调用后打印（可能不崩溃，但必须解释为什么这是 UB）；再用 `-fsanitize=address` 编译一次，观察 ASan 是否报错。今天的 ASan 只做体验，D9 系统练。

要求：`-Wall -Wextra -Wpedantic -Wconversion` 零警告。

## 任务 4：左值 / 右值初步（约 20 min）

只要求建立直觉，写进笔记即可，不写复杂代码：

1. 左值：有身份（能取地址）、能出现在赋值左边；

2. 右值：临时对象、字面量，通常不能取地址；

3. 例子：
   
   ```cpp
   int x = 1;   // x 是左值，1 是右值
   int& r = x;  // 左值引用
   int&& rr = 42; // 右值引用（今天的唯一要求：认识这个语法）
   ```

4. 记录：为什么 `std::move` 本身不移动任何东西（一句话即可，D6/D10 再展开）。

## 任务 5：Linux 基础（约 90 min）

### 5.1 Shell 与文件（30 min）

逐条跑，并把「命令 + 作用 + 你实际看到的输出」记入 `docs/linux-basics.md`：

```bash
ls -la; stat <file>
chmod 600 <file>; chmod 755 <dir>; umask
df -h; du -sh .
find . -name '*.cpp'
grep -rn "TODO" .
ls | wc -l
ps aux | grep <keyword>
```

- 权限位：把 `rwxr-xr-x` 换算成八进制；解释目录的 `x` 位为什么是「能进入/遍历」而不是「能执行」；
- 重定向：`>`、`>>`、`2>`、`&>`、管道 `|` 的差异；
- `find ... -exec` 或 `xargs` 至少用一次。

### 5.2 进程与信号（40 min）

1. 后台跑一个进程：`sleep 300 &`；
2. `ps -o pid,ppid,stat,cmd -p <pid>`、`ps -ef | grep sleep`；
3. 看 `/proc/<pid>/status` 和 `/proc/<pid>/cmdline`；
4. 信号：`kill -l`，记录 `SIGINT(2)`、`SIGTERM(15)`、`SIGKILL(9)`、`SIGHUP(1)`、`SIGPIPE(13)`；
5. 分别用 `kill -TERM` 和 `kill -KILL` 结束进程，记录差别（为什么 `SIGKILL` 不能被捕获/忽略）；
6. 终端里 `Ctrl+C`（SIGINT）和 `Ctrl+Z`（SIGTSTP）各试一次，配合 `jobs`、`bg`、`fg`；
7. `nohup` 或 `&` + `disown` 启动一个不会被终端关闭带走的进程。

### 5.3 strace 入门（20 min）

```bash
strace -f -e trace=openat,read,write,exit_group ./hello
strace -c ./hello
```

观察并写进笔记：

- 程序启动时最先打开的文件（动态链接器 `ld-linux`、`libc`）；
- `write(1, ...)` 中的 `1` 是什么（文件描述符，对应 stdout）；
- `strace -c` 的统计能看出什么；
- 对比 `strace ./hello` 和 `strace -c ./hello` 的输出差异。

> 提示：本地的 `hello` 用 D01 构建出来的可执行文件即可。

## 任务 6：接入 CMake + Git（约 20 min）

- 在 `base/CMakeLists.txt` 中为 `types.cpp`、`pointers.cpp` 各加一个独立可执行 target，要求仍在 `~/build/base` 下构建、零警告；
- 若需要 UBSan/ASan 版本，临时用命令行编译即可，不必改 CMake（D7 再工程化）；
- Git 提交：`d02: 基础类型 + Linux 基础`；
- 笔记完成后可再提交一次 `d02: 笔记与 Linux 记录`。

## 产出清单

- `days/d02-basics/types.cpp`、`pointers.cpp`（可直接构建运行）；
- `days/d02-basics/note.md`（按模板，含 UB 复现与 UBSan 输出）；
- `docs/linux-basics.md`（权限、命令、进程与信号、strace 记录）；
- CMake 中新增两个 target；
- 至少一次 Git 提交。

## 验收标准

- [ ] 两个练习文件在严格警告下零警告编译，运行结果正确；
- [ ] 能解释四种初始化、窄化、`const`/`constexpr` 区别；
- [ ] 能说清 `const int*` / `int* const` / `const int* const` 三者的差异；
- [ ] 能解释数组退化和 `sizeof` 差异；
- [ ] 权限位能口算，`SIGTERM` vs `SIGKILL` 的区别能讲清；
- [ ] 能用 `strace -e trace=...` 定位程序打开了哪些文件。

## 自测问题

1. `int x;` 和 `int x{};` 有什么区别？函数内不初始化直接读会怎样？
2. 列表初始化为什么能防窄化？`int x{3.0}` 和 `int x = 3.0` 哪个安全？
3. `auto a = ci` 后 `a` 的 const 属性去哪了？为什么？
4. `constexpr` 函数在运行时也能调用吗？`consteval` 呢（D7 会讲）？
5. 引用为什么不能为空、不能重新绑定？底层通常怎么实现？
6. 有符号溢出和左移越界为什么是 UB？无符号为什么不是？
7. `char* p = "abc"; p[0] = 'x';` 在 C++20 中错在哪？
8. 目录的 `x` 权限和 `r` 权限分别意味着什么？
9. 为什么 `SIGKILL` 杀不掉处于 D 状态（不可中断睡眠）的进程？
10. `strace -c` 输出里 `calls` 和 `errors` 两列告诉你什么？

## 完成后

告诉我「D02 完成」，并附上：

- 两个练习文件的编译输出（确认零警告）；
- `note.md` 与 `docs/linux-basics.md`；
- `strace -c ./hello` 的完整输出；
- `git log --oneline` 结果。

我会检查代码里的 UB 是否真的被修掉、笔记是否停留在「抄命令」层面，确认后再进入 D03（函数、链接、GDB）。
