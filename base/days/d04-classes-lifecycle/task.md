# D04 任务：类与对象 I —— 生命周期

> 预计 5 小时。概念 1h / 动手 3h / 笔记与 Git 1h。
> 环境：Ubuntu 24.04（WSL2 或 README 里的虚拟机），GCC 13+，CMake 3.28+，Ninja，GDB，ASan。
> 构建目录统一用 `~/build/base`，源码在 `base/days/d04-classes-lifecycle/code/`。

今天第一次自己管理资源。核心问题只有一个：

> **资源什么时候获得、什么时候释放、拷贝时到底复制了什么？**

围绕这个问题，完成三件事：

1. 构造 / 析构 / 初始化列表 / `explicit`；
2. 拷贝构造 / 拷贝赋值 / 浅拷贝与深拷贝；
3. Rule of 0/3/5 与 RAII：手写 `String` 和 `FileHandle`。

## 今日目标

- [ ] 说清构造、析构、成员初始化的顺序
- [ ] 会用初始化列表，知道 `const` / 引用成员为什么必须在列表里初始化
- [ ] 理解 `explicit` 阻止了什么隐式转换
- [ ] 分清拷贝构造和拷贝赋值，能判断每一行代码调用了哪个
- [ ] 能复现浅拷贝导致的 double-free，并用深拷贝修复
- [ ] 能解释 Rule of 0/3/5，知道什么时候该写哪些特殊成员
- [ ] 手写 `String`：深拷贝、自赋值安全、零警告
- [ ] 手写 `FileHandle`：构造打开、析构关闭、禁止拷贝
- [ ] 用 ASan 验证坏拷贝报错、修复后无泄漏
- [ ] CMake 多 target、Git 提交、`note.md` 完成

## 今日不碰

- 继承、多态、虚函数、虚析构（D5）
- 运算符重载（今天的拷贝赋值 `operator=` 除外；`operator<<`、`operator[]` 等 D6）
- 模板、异常、智能指针、多线程
- 移动语义深入：Rule of 5 的移动构造 / 移动赋值今天只解释，不要求实现；D10 再展开

> 本目录的 `code/` 已经建好；所有练习文件放进去。
> 下面的命令如果没有特别说明，默认在 `base/days/d04-classes-lifecycle/code/` 下执行；CMake 命令在仓库根执行。
> 今天会第一次使用 `new[]` / `delete[]`。只用于手写 `String` 和 `FileHandle` 的示例，不要扩散到其他练习。

---

## 任务 1：构造、析构、初始化列表、`explicit`（约 50 min）

### 1.1 构造函数与初始化列表

在 `code/ctors.cpp` 里写三个类：

**（1）`Point`**

```cpp
class Point {
public:
    Point();
    Point(int x, int y);
    void print() const;
private:
    int x_;
    int y_;
};
```

要求：

- 默认构造把 `x_`、`y_` 初始化为 0；
- 带参构造用**初始化列表**，不要先默认构造再在函数体里赋值；
- 打印 `x_`、`y_`。

在笔记里对比：

```cpp
Point::Point(int x, int y) : x_(x), y_(y) {}  // 初始化
Point::Point(int x, int y) { x_ = x; y_ = y; } // 赋值
```

第二种写法对 `int` 可能看不出差别，但如果成员是 `const`、引用或没有默认构造的类类型，函数体赋值就根本不可行。记录这一点。

**（2）`Config`**

```cpp
class Config {
public:
    Config(int id, int& value);
private:
    const int id_;
    int& value_;
};
```

要求：

- 用初始化列表初始化 `id_` 和 `value_`；
- 尝试在构造函数体里给 `id_` 赋值，记录编译错误（放 `code/fail_const_assign.cpp`，不进 CMake）；
- 说明为什么引用成员也必须在初始化列表里绑定。

**（3）`Trace`**

```cpp
class Trace {
public:
    explicit Trace(const char* name);
    ~Trace();
private:
    const char* name_;
};
```

构造函数和析构函数各打印一行。在 `main` 里制造嵌套作用域：

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

记录构造和析构的完整顺序，并回答：

- 局部对象按什么顺序构造、按什么顺序析构？
- 成员对象和对象本身谁先构造、谁先析构？
- 如果类里有多个成员，成员的析构顺序和声明顺序是什么关系？

### 1.2 初始化列表的顺序陷阱

写 `code/warn_reorder.cpp`：

```cpp
class Reorder {
public:
    Reorder(int v) : b_(v), a_(b_) {}
private:
    int a_;
    int b_;
};
```

用严格警告编译，观察 `-Wreorder`：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -c warn_reorder.cpp -o /tmp/warn_reorder.o
```

记录警告原文。解释：

- 初始化列表的书写顺序**不决定**初始化顺序；
- 真正顺序是成员在类中的**声明顺序**；
- `a_(b_)` 在 `b_` 之前执行，读到的是未初始化的 `b_`，这是 UB；所以不要运行这个程序。

### 1.3 `explicit`

写 `code/fail_explicit.cpp`：

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

记录编译错误。然后去掉 `explicit` 再编译一次，确认这两行能通过。回答：

- `explicit` 阻止的是哪一种初始化？
- 为什么单参数构造函数默认加 `explicit` 通常是好习惯？
- `Box b = Box(10);` 和 `Box b(10);` 有什么区别？

### 1.4 默认 / 删除特殊成员

在 `ctors.cpp` 里加一个小类，演示：

```cpp
class NonCopyable {
public:
    NonCopyable() = default;
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
};
```

尝试拷贝它，记录编译错误。说明 `= default` 和 `= delete` 分别是什么意思。

---

## 任务 2：拷贝构造与拷贝赋值（约 60 min）

### 2.1 什么时候调用拷贝构造，什么时候调用拷贝赋值

写 `code/copy_demo.cpp`，定义 `Tracker`：

```cpp
class Tracker {
public:
    Tracker();
    Tracker(const Tracker& other);
    Tracker& operator=(const Tracker& other);
    ~Tracker();
    int id() const;
private:
    int id_;
    static int next_id_;
};
```

`static int next_id_;` 只是声明，记得在 `.cpp` 里定义一次：

```cpp
int Tracker::next_id_ = 0;
```

让拷贝构造、拷贝赋值、析构都打印一行（带 `id_`）。要求观察并记录：

```cpp
Tracker a;              // 默认构造
Tracker b = a;          // 拷贝构造，不是赋值
Tracker c(a);           // 拷贝构造
b = a;                  // 拷贝赋值
```

再写两个函数：

```cpp
void take_by_value(Tracker t);
Tracker make_tracker();   // 实现成 return Tracker{};
```

调用：

```cpp
take_by_value(a);
Tracker d = make_tracker();
```

回答：

- 按值传参调用了几次拷贝构造？
- `make_tracker()` 返回的是临时对象（prvalue），`Tracker d = make_tracker();` 在 C++17 下是否发生拷贝？（提示：guaranteed copy elision）
- 如果 `make_tracker` 改成 `Tracker t; return t;`（返回具名局部变量），拷贝是否被省略？这依赖 NRVO，和上一种情况有什么不同？
- `Tracker b = a;` 和 `b = a;` 的语法差别是什么？为什么前者不是赋值？

### 2.2 浅拷贝：默认拷贝到底复制了什么

写 `code/bad_string.cpp`：

```cpp
class BadString {
public:
    explicit BadString(const char* s);
    ~BadString();
    const char* c_str() const;
private:
    char* data_;
};
```

构造函数里用 `new char[...]` 分配并复制字符串；析构函数里 `delete[] data_`；**不写拷贝构造和拷贝赋值**。

在 `main` 里：

```cpp
BadString a("hello");
BadString b = a;   // 编译器生成的拷贝构造：只复制指针
```

运行时会发生什么？用 ASan 构建并运行：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -g \
    -fsanitize=address -fno-omit-frame-pointer \
    bad_string.cpp -o /tmp/bad_string_asan
/tmp/bad_string_asan
```

记录 ASan 的完整错误（`attempting double-free` 或 `heap-use-after-free`）。在笔记里画出：

```text
a.data_ ──┐
          ├──> [h e l l o \0]
b.data_ ──┘
```

解释：

- 编译器生成的拷贝构造是逐成员拷贝，指针只复制地址；
- 两个对象都认为自己拥有同一块内存；
- 第一个析构 `delete[]` 后，第二个再 `delete[]` 就是 double-free。

再说明：如果不写拷贝赋值，`b = a;` 也会产生同样的问题（还可能泄漏 `b` 原来的内存）。

### 2.3 为什么 `new[]` 必须配 `delete[]`

记录并解释：

- `new[]` 分配的是数组，运行时需要知道元素个数才能正确析构/释放；
- `new` / `delete` 与 `new[]` / `delete[]` 不能混用；
- `std::malloc` / `std::free` 是另一套，不能和 `new` / `delete` 混用。

---

## 任务 3：Rule of 0/3/5 与手写 `String`（约 90 min）

### 3.1 先判断：这个类需要哪些特殊成员？

在笔记里填表：

| 类的成员 | 需要手写析构？ | 需要手写拷贝？ | 需要手写移动？ | 属于 Rule of ? |
|---|---|---|---|---|
| `int`、`double` 等值类型 |  |  |  |  |
| `std::string`、`std::vector` |  |  |  |  |
| 裸 `char*` + `new[]` |  |  |  |  |
| `FILE*` / 文件描述符 |  |  |  |  |

结论：

- **Rule of 0**：成员自己管理资源（如 `std::string`），编译器生成的析构/拷贝/移动都正确，不要手写。
- **Rule of 3**：手写了析构、拷贝构造、拷贝赋值中的任何一个，通常三个都要写。
- **Rule of 5**：在 Rule of 3 基础上再加移动构造、移动赋值；今天只解释，不实现。

### 3.2 `String` 的接口

在 `code/` 下建三个文件：

```text
my_string.h
my_string.cpp
string_main.cpp
```

`my_string.h`：

```cpp
#pragma once
#include <cstddef>

class String {
public:
    String();
    explicit String(const char* s);
    String(const String& other);
    String& operator=(const String& other);
    ~String();

    const char* c_str() const;
    std::size_t size() const;
    bool empty() const;

private:
    char* data_;
    std::size_t size_;
};
```

要求：

- `size_` 不包含结尾的 `'\0'`；
- 默认构造也保证 `c_str()` 返回合法空串（可以分配 1 字节放 `'\0'`）；
- 带参构造用 `std::strlen` 求长度，`new char[size_ + 1]`，复制内容并补 `'\0'`；
- 拷贝构造：**重新分配**一块内存，再复制内容（深拷贝）；
- 拷贝赋值：
  - 先处理自赋值 `this == &other`；
  - 先分配并复制新内存，再释放旧内存，避免“先 delete 后复制”导致用到已释放内存；
  - 返回 `*this`，支持链式赋值；
- 析构：`delete[] data_`；
- 不使用 `std::string` 作为成员（否则就不是手写资源管理了）。

### 3.3 必做测试

`string_main.cpp` 至少覆盖：

```cpp
String a("hello");
String b = a;             // 拷贝构造
String c;                 // 默认构造
c = a;                    // 拷贝赋值
c = c;                    // 自赋值
String d("x");
d = b = a;                // 链式赋值
```

验证：

- `a.c_str()` 和 `b.c_str()` 的内容相同，但地址不同（深拷贝）；
- `c = c` 后内容仍正确，没有崩溃；
- `empty()`、`size()` 对空串和普通串都正确；
- 所有对象析构时 ASan 不报错、不泄漏。

记录 `a.c_str()`、`b.c_str()` 的地址差异。

### 3.4 Rule of 0 对照

写 `code/rule_of_zero.cpp`：

```cpp
class Person {
public:
    Person(const std::string& name, int age) : name_(name), age_(age) {}
    const std::string& name() const { return name_; }
    int age() const { return age_; }
private:
    std::string name_;
    int age_;
};
```

不写析构、拷贝、移动，拷贝一个 `Person`，确认 `name_` 被正确复制。解释：`std::string` 自己遵守 Rule of 0/3/5，`Person` 只需要让编译器生成即可。

### 3.5 Rule of 5 只解释，不实现

在笔记里写清五个特殊成员：

```cpp
~String();
String(const String&);            // 拷贝构造
String& operator=(const String&); // 拷贝赋值
String(String&&) noexcept;        // 移动构造（D10）
String& operator=(String&&) noexcept; // 移动赋值（D10）
```

回答：

- 为什么 `String` 写了析构和拷贝后，编译器不会自动生成移动成员？
- 移动构造通常做什么？（偷走指针，把源对象置空）
- 为什么移动构造函数常标 `noexcept`？
- 为什么今天不实现移动也能通过所有测试？（rvalue 会退化为 const 引用拷贝；D10 再优化）

---

## 任务 4：`FileHandle` 与 RAII（约 50 min）

### 4.1 手动管理文件：早期 return 的坑

先写一个“反面教材”（可以只放在笔记里，不必编译）：

```cpp
std::FILE* fp = std::fopen("/tmp/d04_manual.txt", "w");
if (!fp) return;
std::fputs("first\n", fp);
if (some_condition) return;  // 忘记 fclose，文件描述符泄漏
std::fclose(fp);
```

解释：

- 每个 `return`、每条异常路径都必须记得释放；
- 代码一复杂就会漏；
- RAII 把“释放”绑定到对象析构，离开作用域自动执行。

### 4.2 实现 `FileHandle`

在 `code/` 下建：

```text
file_handle.h
file_handle.cpp
file_main.cpp
```

`file_handle.h`：

```cpp
#pragma once
#include <cstdio>

class FileHandle {
public:
    explicit FileHandle(const char* path, const char* mode = "r");
    ~FileHandle();

    FileHandle(const FileHandle&) = delete;
    FileHandle& operator=(const FileHandle&) = delete;

    bool is_open() const;
    std::FILE* get() const;
    bool write(const char* text);

private:
    std::FILE* fp_;
};
```

要求：

- 构造函数用初始化列表 `fp_(std::fopen(path, mode))`；
- 析构函数：`if (fp_) std::fclose(fp_);`；
- `is_open()` 返回 `fp_ != nullptr`；
- `get()` 返回底层指针，便于调用 `std::fputs` 等 C API；
- `write()` 用 `std::fputs`，失败返回 `false`；
- 拷贝构造和拷贝赋值用 `= delete`，防止两个对象关闭同一个 `FILE*`。

### 4.3 验证 RAII

`file_main.cpp` 里写一个函数：

```cpp
void write_then_return() {
    FileHandle f("/tmp/d04_filehandle.txt", "w");
    if (!f.is_open()) {
        return;
    }
    f.write("first line\n");
    return;  // 析构仍然会执行
}
```

要求：

- 调用后 `cat /tmp/d04_filehandle.txt`，能看到 `first line`；
- 在函数中间加一个提前 `return`，确认析构仍然执行；
- 尝试拷贝 `FileHandle`，记录编译错误（`use of deleted function`）；
- 解释为什么 `= delete` 比“写一个空实现的拷贝构造”更安全。

### 4.4 可选：观察文件描述符

在 Linux 上可以用 `/proc/self/fd` 观察打开的文件描述符数量：

```bash
ls /proc/self/fd
```

如果写一个循环反复 `fopen` 不 `fclose`，最终会耗尽 fd；用 RAII 版本则不会。这个实验可选，不要跑太多导致系统资源紧张。

---

## 任务 5：CMake、ASan、Git、笔记（约 40 min）

### 5.1 CMake targets

在 `base/CMakeLists.txt` 中为每个带 `main` 的练习文件加独立 target：

| target | 源文件 |
|---|---|
| `d04-ctors` | `ctors.cpp` |
| `d04-copy` | `copy_demo.cpp` |
| `d04-string` | `string_main.cpp` + `my_string.cpp` |
| `d04-ruleofzero` | `rule_of_zero.cpp` |
| `d04-filehandle` | `file_main.cpp` + `file_handle.cpp` |

要求：

- C++20 + `-Wall -Wextra -Wpedantic -Wconversion`；
- `fail_explicit.cpp`、`fail_const_assign.cpp`、`warn_reorder.cpp`、`bad_string.cpp` **不要**加进 CMake；
- 零警告构建。

```bash
cmake -S /mnt/d/Project/CPP/CPPLearn/base -B ~/build/base
cmake --build ~/build/base --target d04-ctors d04-copy d04-string d04-ruleofzero d04-filehandle -j"$(nproc)"
```

### 5.2 ASan 验证

坏拷贝（应报 double-free）：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -g \
    -fsanitize=address -fno-omit-frame-pointer \
    bad_string.cpp -o /tmp/bad_string_asan
/tmp/bad_string_asan
```

修复后的 `String`（应无错误、无泄漏）：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -g \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    string_main.cpp my_string.cpp -o /tmp/string_asan
/tmp/string_asan
```

`FileHandle`：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -g \
    -fsanitize=address,undefined -fno-omit-frame-pointer \
    file_main.cpp file_handle.cpp -o /tmp/filehandle_asan
/tmp/filehandle_asan
```

要求记录：

- 坏拷贝的 ASan 错误类型和调用栈；
- 修复后 `String` 的 ASan 输出为空、退出码 0；
- `FileHandle` 的 ASan 输出为空；
- 如果出现 leak，指出哪次 `new[]` 没有对应的 `delete[]`。

### 5.3 Git

```bash
git switch -c d04-classes-lifecycle   # 如果 D03 已经用分支流程
git add base/days/d04-classes-lifecycle base/CMakeLists.txt
git commit -m "d04: 类与对象 I：生命周期"
git switch main
git merge --no-ff d04-classes-lifecycle -m "merge: d04 类与对象 I"
git branch -d d04-classes-lifecycle
```

如果继续在 `main` 上提交，至少保证提交信息是 `d04: 类与对象 I：生命周期`，并且 `git status` 干净。

### 5.4 `note.md`

按模板写，至少包含：

- 构造 / 析构顺序的实测输出；
- 初始化列表 vs 函数体赋值；`const` / 引用成员；
- `-Wreorder` 的警告和原因；
- `explicit` 的编译错误；
- 拷贝构造 / 拷贝赋值的调用矩阵；
- 浅拷贝图、double-free 的 ASan 输出；
- Rule of 0/3/5 对照表；
- `String` 的实现要点：深拷贝、自赋值、先分配后释放；
- `FileHandle` 的 RAII 设计和 `= delete`；
- 3–5 条易错点。

---

## 产出清单

- `code/ctors.cpp`、`copy_demo.cpp`、`rule_of_zero.cpp`；
- `code/my_string.h`、`my_string.cpp`、`string_main.cpp`；
- `code/file_handle.h`、`file_handle.cpp`、`file_main.cpp`；
- 失败/告警实验：`fail_explicit.cpp`、`fail_const_assign.cpp`、`warn_reorder.cpp`、`bad_string.cpp`；
- `note.md`；
- `base/CMakeLists.txt` 中 5 个新 target；
- 至少一次 Git 提交。

## 验收标准

- [ ] 能画出构造 / 析构顺序，解释成员初始化顺序由声明顺序决定；
- [ ] 能解释 `const` / 引用成员为什么必须在初始化列表初始化；
- [ ] 能说清 `explicit` 阻止了哪种隐式转换；
- [ ] 能准确判断 `Tracker b = a;` 和 `b = a;` 调用的是拷贝构造还是拷贝赋值；
- [ ] 能解释浅拷贝为什么 double-free，并画出指针图；
- [ ] `String` 深拷贝正确，自赋值安全，ASan 无错误；
- [ ] `FileHandle` 在提前 `return` 时仍然关闭文件，拷贝被 `= delete` 阻止；
- [ ] 能说清 Rule of 0/3/5，知道今天为什么只实现 Rule of 3；
- [ ] 严格警告下零警告，CMake 五个 target 都能构建运行。

## 自测问题

1. 初始化列表和构造函数体赋值，对 `const` 成员、引用成员分别意味着什么？
2. 成员的初始化顺序由什么决定？初始化列表的书写顺序有影响吗？
3. `explicit Box(int)` 阻止了哪些写法？为什么单参数构造函数常加 `explicit`？
4. `Tracker b = a;` 和 `b = a;` 分别调用哪个特殊成员？
5. 按值传参、按值返回分别可能触发几次拷贝？C++17 的 guaranteed copy elision 保证了什么？
6. 编译器生成的拷贝构造对裸指针做了什么？为什么会导致 double-free？
7. Rule of 0、Rule of 3、Rule of 5 分别是什么？什么时候该用哪个？
8. 写了析构函数后，编译器还会自动生成移动构造吗？为什么？
9. 自赋值 `s = s;` 如果不检查会发生什么？先分配后释放的顺序为什么更安全？
10. `delete` 和 `delete[]` 为什么不能混用？
11. RAII 的核心思想是什么？`FileHandle` 的析构函数在什么时机执行？
12. 为什么 `FileHandle` 要 `= delete` 拷贝构造和拷贝赋值？如果允许拷贝会发生什么？
13. ASan 报 `attempting double-free` 和 `LeakSanitizer: detected memory leaks` 分别说明什么？
14. 今天为什么只实现 Rule of 3？D10 的移动构造会带来什么改进？

## 完成后

告诉我「D04 完成」，并附上：

- `cmake --build` 的完整输出（确认零警告）；
- `bad_string` 的 ASan 报错，以及修复后 `String` 的干净 ASan 输出；
- `note.md`，特别是构造/析构顺序、拷贝调用矩阵、Rule of 0/3/5 对照表；
- `git log --oneline --graph`。

我会检查 `String` 的自赋值和深拷贝是否真的正确、`FileHandle` 是否在所有提前返回路径都能关闭文件；确认后再进入 D05（继承与多态）。
