# D04 任务 4：`FileHandle` 与 RAII

> 环境：WSL2 Ubuntu 24.04，g++ 13.3.0。
> 代码目录：[code/](code/)
> 本节的输出都来自实际编译、运行；`FileHandle` 通过了 ASan + UBSan，退出码 0。

## 0. 速查

- **RAII** = Resource Acquisition Is Initialization：资源在构造时获得，在析构时释放。
- 栈对象离开作用域**一定**析构，包括提前 `return`、抛异常展开栈帧。
- `FileHandle`：构造 `fopen`，析构 `if (fp_) fclose(fp_)`。
- 拷贝构造 / 拷贝赋值用 `= delete`：两个对象关闭同一个 `FILE*` 就是 double close，属于 UB。
- `= delete` 在**编译期**报错；写成空实现的拷贝会把灾难留到运行期。
- 析构函数必须判 `fp_` 是否为空，`fclose(nullptr)` 是 UB。

## 1. 文件清单

| 文件 | 内容 | 结果 |
|---|---|---|
| `file_handle.h` | `FileHandle` 接口，拷贝成员 `= delete` | — |
| `file_handle.cpp` | 构造 `fopen`、析构 `fclose`、`is_open` / `get` / `write` | 零警告 |
| `file_main.cpp` | 提前 `return` 仍关闭文件、重新打开验证 flush | 运行通过，ASan/UBSan 干净 |
| `fail_filehandle_copy.cpp` | 故意拷贝，记录编译错误 | 编译失败（预期），不进 CMake |

## 2. 构建与运行

```bash
cd base/days/d04-classes-lifecycle/code

g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    file_main.cpp file_handle.cpp -o /tmp/d04_filehandle
/tmp/d04_filehandle
cat /tmp/d04_filehandle.txt

g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    file_main.cpp file_handle.cpp -o /tmp/filehandle_asan
/tmp/filehandle_asan
```

> **记忆点：** RAII 的验收方式是看“离开作用域后资源有没有释放”，而不是看代码里有没有写 `fclose`。

## 3. 手动管理资源的问题

### 3.1 反面教材

```cpp
std::FILE* fp = std::fopen("/tmp/d04_manual.txt", "w");
if (!fp) return;
std::fputs("first\n", fp);
if (some_condition) return;   // 忘记 fclose，文件描述符泄漏
std::fclose(fp);
```

每一条 `return`、每一条异常路径都必须记得释放，代码一复杂就一定会漏。手工管理的核心问题是：**资源的生命周期靠人记忆，而不是靠语言规则**。

### 3.2 实测：fd 泄漏与 RAII 对比

用 `/proc/self/fd` 统计进程打开的文件描述符数量：

```cpp
int count_fds() {
    DIR* d = opendir("/proc/self/fd");
    if (!d) return -1;
    int n = 0;
    while (readdir(d) != nullptr) ++n;
    closedir(d);
    return n;   // 含 .、.. 和 opendir 自己占的 fd
}

for (int i = 0; i < 5; ++i) {
    std::FILE* fp = std::fopen("/tmp/d04_fd_leak.txt", "w");
    (void)fp;   // 故意不 fclose
}
// 对照：FileHandle 离开循环体就关闭
for (int i = 0; i < 5; ++i) {
    FileHandle f("/tmp/d04_fd_raii.txt", "w");
}
```

实际输出：

```text
初始 fd 数量: 7
5 次 fopen 不 fclose 后: 12
5 次 RAII 打开关闭后: 12
```

5 次不关闭的 `fopen` 让 fd 数量从 7 涨到 12，泄漏的 5 个 fd 一直没回来；RAII 版本开 5 次、关 5 次，数量保持 12 不变。如果循环足够多，最终会耗尽进程的 fd 上限（`ulimit -n`）。

> **记忆点：** 手工释放的总会漏在某个 `return` / 异常分支上；RAII 让“释放”绑定到析构，编译器保证离开作用域时执行。

## 4. `FileHandle` 的实现

### 4.1 构造函数：用初始化列表获得资源

```cpp
FileHandle::FileHandle(const char* path, const char* mode)
    : fp_(std::fopen(path, mode)) {}
```

`fp_` 在初始化列表里直接拿到 `fopen` 的返回值，构造完成时资源已经就位——这正是 RAII 名字的含义：资源的获取就是初始化的一部分。

即使 `fopen` 失败，对象也是合法构造的：`fp_ == nullptr`，`is_open()` 返回 `false`。这种设计把“打开失败”变成对象的一个状态，而不是构造异常。

> **记忆点：** RAII 的构造函数要么成功拿到资源，要么让对象处于一个可检查的“空”状态；本例选用后者。

### 4.2 析构函数：释放资源

```cpp
FileHandle::~FileHandle() {
    if (fp_) {
        std::fclose(fp_);
    }
}
```

- `fclose` 会 flush 缓冲区并释放文件描述符；
- 必须先判 `fp_`：`fclose(nullptr)` 是 UB，打开失败的对象也会在作用域结束时析构；
- 析构函数不抛异常（`fclose` 失败也不能让异常逃出析构）。

> **记忆点：** 析构里先判空再释放；打开失败的对象也必须能安全析构。

### 4.3 访问接口

```cpp
bool is_open() const { return fp_ != nullptr; }
std::FILE* get() const { return fp_; }

bool write(const char* text) {
    if (!fp_) {
        return false;
    }
    return std::fputs(text, fp_) != EOF;
}
```

`get()` 暴露底层 `FILE*`，方便调用 `std::fputs` 等 C API；`write()` 返回 `bool`，失败时调用方可以处理，而不是让程序直接崩。

> **记忆点：** `get()` 只借出句柄，不转移所有权；对象的析构仍然负责 `fclose`。

### 4.4 为什么要 `= delete` 拷贝

```cpp
FileHandle(const FileHandle&) = delete;
FileHandle& operator=(const FileHandle&) = delete;
```

`FILE*` 是**独占资源**：一个 `FILE*` 只能被 `fclose` 一次。如果允许拷贝，两个 `FileHandle` 会持有同一个 `fp_`：

```text
a.fp_ ──┐
        ├──> FILE*（同一个文件流）
b.fp_ ──┘
```

作用域结束时两个析构函数都会 `fclose` 同一个指针：第一次关闭成功，第二次就是 double close，属于 UB，可能破坏 C 运行库的内部状态。这和浅拷贝的 double-free 是同一类问题。

`= delete` 把错误提前到编译期：任何拷贝尝试都无法通过编译，而且错误信息明确指向被删除的函数。

> **记忆点：** 独占资源禁止拷贝；`= delete` 在编译期拦截，比空实现（运行期 double close）安全得多。

## 5. RAII 验证

### 5.1 提前 `return` 仍会析构

`file_main.cpp`：

```cpp
void write_then_return() {
    FileHandle f(kPath, "w");
    if (!f.is_open()) {
        return;
    }
    f.write("first line\n");
    return;   // f 在这里析构 -> fclose -> flush
}
```

实际输出：

```text
write_then_return: 提前 return
重新打开文件读取（能读到内容说明已 fclose/flush）：
  first line
追加一行后再次读取：
  first line
  second line
```

函数里没有任何显式 `fclose`，而且是在 `return` 之前就写入了数据。随后**重新打开**文件能立即读到 `first line`，说明 `f.write` 的 stdio 缓冲已经被 `fclose` flush 给操作系统——也就是析构确实执行了。

`cat /tmp/d04_filehandle.txt` 也确认内容：

```text
first line
second line
```

### 5.2 ASan + UBSan

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -g \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    file_main.cpp file_handle.cpp -o /tmp/filehandle_asan
/tmp/filehandle_asan
```

没有任何额外报错，退出码 0，没有 fd 或内存相关的问题。

### 5.3 拷贝的编译错误

`fail_filehandle_copy.cpp`：

```cpp
FileHandle a("/tmp/d04_filehandle.txt");
FileHandle b = a;   // 错误：拷贝构造被 delete
FileHandle c("/tmp/d04_other.txt");
c = a;              // 错误：拷贝赋值被 delete
```

实际报错：

```text
fail_filehandle_copy.cpp: In function ‘int main()’:
fail_filehandle_copy.cpp:7:20: error: use of deleted function ‘FileHandle::FileHandle(const FileHandle&)’
    7 |     FileHandle b = a;   // 错误：拷贝构造被 delete
      |                    ^
In file included from fail_filehandle_copy.cpp:3:
file_handle.h:9:5: note: declared here
    9 |     FileHandle(const FileHandle&) = delete;
      |     ^~~~~~~~~~
fail_filehandle_copy.cpp:9:9: error: use of deleted function ‘FileHandle& FileHandle::operator=(const FileHandle&)’
    9 |     c = a;              // 错误：拷贝赋值被 delete
      |         ^
file_handle.h:10:17: note: declared here
   10 |     FileHandle& operator=(const FileHandle&) = delete;
      |                 ^~~~~~~~
```

两条错误都直接指出“调用了被删除的函数”，并给出声明的行号。

### 5.4 为什么 `= delete` 比空实现安全

假设写成空实现的拷贝构造：

```cpp
FileHandle(const FileHandle&) {}   // 错误示范
```

- `fp_` 没有被初始化（内置指针默认初始化 → 不确定值），析构时 `if (fp_)` 可能为非空 → `fclose` 一个垃圾地址；
- 如果写成 `fp_ = other.fp_;`，两个对象共享同一个 `FILE*`，析构时 double close；
- 编译器不会阻止拷贝，错误要等到运行期才暴露。

`= delete` 的版本：

- 函数仍然被声明、参与重载解析，任何拷贝尝试都在编译期报错；
- 错误信息清楚，直接定位到 `= delete` 那一行；
- 程序根本不可能带着“共享句柄”的状态被构建出来。

> **记忆点：** `= delete` 是“编译期拒绝”，空实现是“运行期爆炸”；独占资源宁可不让它编译通过。

## 6. 易错点

1. **析构不判 `fp_`**：`fclose(nullptr)` 是 UB，打开失败的对象也会析构。
2. **拷贝写成空实现**：要么 `fp_` 未初始化、析构时乱关，要么两个对象 double close。
3. **以为提前 `return` 不会释放**：栈对象析构由作用域决定，`return` 只是离开作用域的一种方式。
4. **忘记 `fclose` 会 flush**：文件内容能立刻被重新读到，正是析构里 `fclose` 的功劳。
5. **构造失败时抛异常**：本例选择“空对象 + `is_open()`”而不是抛异常；两种风格都可以，但要统一。
6. **在析构里抛异常**：析构函数应当 `noexcept`，`fclose` 失败也只能忽略或记录。
7. **以为 `FileHandle` 可以 `std::move`**：拷贝构造被 `= delete` 后，隐式移动构造也不会生成，所以它既不可拷贝也不可移动；需要转移所有权时要自己写移动构造。
8. **把 `get()` 当成所有权转移**：返回的 `FILE*` 仍然由 `FileHandle` 负责关闭，调用方不能自己 `fclose`。

## 7. 待复习

- RAII 的核心思想是什么？资源获取和释放分别绑定在哪个函数上？
- 函数提前 `return` 时，局部 `FileHandle` 的析构还会执行吗？怎么验证？
- 为什么 `FileHandle` 的析构要先判断 `fp_`？`fclose(nullptr)` 会怎样？
- 为什么 `FileHandle` 要 `= delete` 拷贝构造和拷贝赋值？允许拷贝会发生什么？
- `= delete` 和“空实现的拷贝构造”在出错时机上有什么区别？
- 为什么 `FileHandle` 既不能拷贝也不能移动？如果想支持移动，要写什么？
- 析构函数里能不能抛异常？为什么？
