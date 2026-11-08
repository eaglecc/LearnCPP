# D04 任务 2：拷贝构造与拷贝赋值

> 环境：WSL2 Ubuntu 24.04，g++ 13.3.0。
> 代码目录：[code/](code/)
> 任务 2 的代码已补齐，下面的输出、ASan 报错都来自实际编译和运行；`bad_string.cpp` 是故意失败的实验，不进 CMake。

## 0. 速查

- `T b = a;`、`T b(a);` 都是**拷贝构造**；`b = a;`（`b` 已存在）才是**拷贝赋值**。
- 拷贝构造负责“新对象出生时复制”，拷贝赋值负责“已有对象换内容”。
- 编译器生成的拷贝是**逐成员复制**：裸指针只复制地址，也就是浅拷贝。
- 浅拷贝的两个对象析构时各 `delete[]` 一次同一块内存 → double-free。
- 按值传参调用一次拷贝构造；C++17 按值返回 prvalue **保证不拷贝**，返回具名局部变量靠 NRVO（可选）。
- `new[]` 必须配 `delete[]`；混用是 UB，ASan 报 `alloc-dealloc-mismatch`。
- 只要类自己管理了资源，写了析构 / 拷贝构造 / 拷贝赋值中的任何一个，通常三个都要写（Rule of 3，任务 3 展开）。

## 1. 文件清单

| 文件 | 内容 | 结果 |
|---|---|---|
| `copy_demo.cpp` | `Tracker` + 拷贝调用矩阵 + 按值传参 / 返回 | 编译运行成功，零警告 |
| `bad_string.cpp` | 不写拷贝成员的 `BadString`，浅拷贝反面教材 | ASan 报 `double-free`（预期），不进 CMake |

## 2. 构建与运行

```bash
cd base/days/d04-classes-lifecycle/code

g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    copy_demo.cpp -o /tmp/d04_copy
/tmp/d04_copy

g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    bad_string.cpp -o /tmp/d04_bad_string
```

ASan 构建：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -g \
    -fsanitize=address -fno-omit-frame-pointer \
    bad_string.cpp -o /tmp/bad_string_asan
/tmp/bad_string_asan
```

> **记忆点：** 拷贝相关的实验一定要用 ASan 跑，否则 double-free 可能“看起来正常”，错误被延迟到程序退出或干脆不报。

## 3. 拷贝构造 vs 拷贝赋值

### 3.1 调用矩阵

`copy_demo.cpp` 里 `Tracker` 的四个特殊成员都打印一行，`id_` 由 `static next_id_` 递增分配：

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

int Tracker::next_id_ = 0;
```

`main` 的前四行：

```cpp
Tracker a;              // 默认构造
Tracker b = a;          // 拷贝构造，不是赋值
Tracker c(a);           // 拷贝构造
b = a;                  // 拷贝赋值
```

实际输出：

```text
== 基本四种写法 ==
默认构造   id=0
拷贝构造   id=1 <- id=0
拷贝构造   id=2 <- id=0
拷贝赋值   id=1 <- id=0
```

逐行对应：

| 写法 | 对象状态 | 调用的成员 | 输出 |
|---|---|---|---|
| `Tracker a;` | 新对象 | 默认构造 | `默认构造 id=0` |
| `Tracker b = a;` | 新对象 `b` | **拷贝构造** | `拷贝构造 id=1 <- id=0` |
| `Tracker c(a);` | 新对象 `c` | **拷贝构造** | `拷贝构造 id=2 <- id=0` |
| `b = a;` | `b` 已存在 | **拷贝赋值** | `拷贝赋值 id=1 <- id=0` |

其中 `b` 的 `id_` 一直是 1：拷贝赋值没有重新分配身份，只是把 `a` 的内容写进 `b`。本例的 `id_` 只是“对象编号”，所以 `operator=` 只打印、不复制 `id_`；真实类里这里复制的是数据或资源。

### 3.2 `Tracker b = a;` 和 `b = a;` 的语法差别

```cpp
Tracker b = a;   // 声明 + 初始化：b 是新对象，调用拷贝构造
b = a;           // 表达式语句：b 已经存在，调用 operator=
```

`=` 只是语法符号，关键看它在**声明**里还是**语句**里：

- 出现在声明里，`=` 是**复制初始化**（copy-initialization），走构造函数；
- 出现在普通语句里，`=` 是**赋值运算符**，对象必须已经构造过。

所以 `Tracker b = a;` 虽然长得像赋值，实际一次 `operator=` 都不会调用。

> **记忆点：** 判断拷贝构造还是拷贝赋值，看左边对象是不是**正在出生**：声明里的 `=` 是出生（拷贝构造），已有对象的 `=` 是换内容（拷贝赋值）。

## 4. 按值传参、按值返回与复制消除

### 4.1 按值传参

```cpp
void take_by_value(Tracker t) {
    std::cout << "  take_by_value 函数体  t.id=" << t.id() << '\n';
}

take_by_value(a);
```

实际输出：

```text
== 按值传参 ==
拷贝构造   id=3 <- id=0
  take_by_value 函数体  t.id=3
析构       id=3
```

形参 `t` 是**新对象**，用实参 `a` 拷贝构造出来；函数返回时 `t` 析构。所以按值传参固定多一次拷贝构造，传参本身也可能有开销——这也是后面要学移动语义的动机之一。

> **记忆点：** 按值传参 = 形参是新对象 = 一次拷贝构造 + 函数结束时一次析构。

### 4.2 按值返回：prvalue（C++17 保证不拷贝）

```cpp
Tracker make_tracker() {
    return Tracker{};   // prvalue
}

Tracker d = make_tracker();
```

实际输出：

```text
== 按值返回：prvalue ==
默认构造   id=4
```

只有一次默认构造，没有拷贝构造。C++17 起，返回 prvalue 时 `Tracker{}` 直接初始化目标对象 `d`，**这不是优化省略，而是语言保证**（guaranteed copy elision）：prvalue 根本不会先变成一个临时对象。

### 4.3 按值返回：具名局部变量（NRVO，可选）

```cpp
Tracker make_tracker_named() {
    Tracker t;
    return t;   // NRVO
}

Tracker e = make_tracker_named();
```

普通编译下实际输出：

```text
== 按值返回：具名局部变量 ==
默认构造   id=5
```

同样没有拷贝。但这是 **NRVO（Named Return Value Optimization）**，属于编译器允许但**不保证**的优化。用 `-fno-elide-constructors` 关掉复制消除再跑：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -fno-elide-constructors copy_demo.cpp -o /tmp/d04_copy_noelide
/tmp/d04_copy_noelide
```

这一段变成：

```text
== 按值返回：具名局部变量 ==
默认构造   id=5
拷贝构造   id=6 <- id=5
析构       id=5
```

出现一次拷贝构造：`return t;` 把局部变量 `t` 拷贝进返回值对象，随后 `t` 析构。注意即使关掉优化，prvalue 那段仍然没有拷贝——**保证消除**和**可选 NRVO** 是两回事。

| 返回写法 | 正常编译 | `-fno-elide-constructors` | 依据 |
|---|---|---|---|
| `return Tracker{};` | 0 次拷贝 | 0 次拷贝 | C++17 保证 |
| `Tracker t; return t;` | 0 次拷贝 | 1 次拷贝 | NRVO 可选 |

> **记忆点：** `return T{};` 在 C++17 保证不拷贝；`return t;`（具名变量）只是 NRVO 优化，关掉优化就能看到拷贝。

## 5. 浅拷贝与 double-free

### 5.1 反面教材 `BadString`

```cpp
class BadString {
public:
    explicit BadString(const char* s) {
        const std::size_t n = std::strlen(s);
        data_ = new char[n + 1];
        std::memcpy(data_, s, n + 1);
    }
    ~BadString() { delete[] data_; }
    const char* c_str() const { return data_; }
private:
    char* data_;
};
```

没有写拷贝构造和拷贝赋值。`BadString b = a;` 会调用**编译器隐式生成**的拷贝构造，它的行为等价于逐成员复制：

```cpp
BadString::BadString(const BadString& other) : data_(other.data_) {}  // 编译器生成
```

实际运行（ASan 构建）先打印地址：

```text
a: hello  addr=0x502000000010
b: hello  addr=0x502000000010
两个对象的 data_ 指向同一块内存
```

两个地址完全相同：只复制了指针的值，没有复制指针指向的字符数组。

### 5.2 指针图

```text
a.data_ ──┐
          ├──> [h e l l o \0]   （同一块堆内存）
b.data_ ──┘
```

这块内存只有一份。`a`、`b` 的析构函数都会执行 `delete[] data_`，而两个 `data_` 指向同一地址。

### 5.3 ASan 实测报错

```text
=================================================================
==2757==ERROR: AddressSanitizer: attempting double-free on 0x502000000010 in thread T0:
    #0 ... in operator delete[](void*)
    #1 ... in BadString::~BadString() .../bad_string.cpp:14
    #2 ... in main .../bad_string.cpp:31
0x502000000010 is located 0 bytes inside of 6-byte region [0x502000000010,0x502000000016)
freed by thread T0 here:
    #0 ... in operator delete[](void*)
    #1 ... in BadString::~BadString() .../bad_string.cpp:14
previously allocated by thread T0 here:
    #0 ... in operator new[](unsigned long)
    #1 ... in BadString::BadString(char const*) .../bad_string.cpp:10
SUMMARY: AddressSanitizer: double-free ... in operator delete[](void*)
==2757==ABORTING
```

调用栈说明三件事：内存由构造函数里的 `new[]` 分配；第一次 `delete[]` 已经把它释放；第二个析构函数再 `delete[]` 同一地址，被 ASan 判定为 double-free。

析构顺序是先 `b` 后 `a`（后构造的先析构），所以是 `b` 先释放，`a` 再释放时出事；换过来结果一样，只是报错位置对调。

> **记忆点：** 编译器生成的拷贝构造是逐成员复制；裸指针只复制地址，两个对象都以为自己独占那块内存，第二次 `delete[]` 就是 double-free。

### 5.4 拷贝赋值也有同样的问题

编译器隐式生成的拷贝赋值同样是逐成员复制：

```cpp
BadString& BadString::operator=(const BadString& other) {
    data_ = other.data_;   // 只覆盖指针
    return *this;
}
```

```cpp
BadString a("hello");
BadString b("world");
b = a;   // 不写 operator= 时走隐式版本
```

实测这个变体同样报 `attempting double-free on 0x502000000010`。而且它还多一层问题：`b` 原来的 `"world"` 指针被直接覆盖，没人再 `delete[]` 它 → **内存泄漏**。不过这次输出里看不到 `LeakSanitizer`，因为程序在 double-free 处就被 abort 了，泄漏检查还没机会跑。

正确做法（任务 3 的 `String`）：

- 拷贝构造：重新 `new[]` 一块内存，再复制内容（深拷贝）；
- 拷贝赋值：先判自赋值，先分配并复制新内存，再释放旧内存，最后返回 `*this`。

> **记忆点：** 不写拷贝赋值时，隐式版本既会造成 double-free，又会让被赋值对象原来的内存泄漏；深拷贝 + 先分配后释放才能同时解决。

## 6. 为什么 `new[]` 必须配 `delete[]`

### 6.1 配对规则

- `new` ↔ `delete`，`new[]` ↔ `delete[]`，两套不能交叉。
- `std::malloc` ↔ `std::free` 是另一套，也不能和 `new` / `delete` 混用。
- 对类类型数组，`delete[]` 会按元素个数逐个调用析构函数；`delete`（非数组形式）不会。元素个数从哪里来，由运行时的数组分配记录（常说的 array cookie）负责，所以分配和释放必须用同一套。

### 6.2 ASan 实测

```cpp
int* p = new int[4]{1, 2, 3, 4};
delete p;   // 错误：new[] 配了 delete
```

```text
==2724==ERROR: AddressSanitizer: alloc-dealloc-mismatch (operator new [] vs operator delete) on 0x502000000010
    #0 ... in operator delete(void*, unsigned long)
    #1 ... in main .../new_delete_mismatch.cpp:5
allocated by thread T0 here:
    #0 ... in operator new[](unsigned long)
    #1 ... in main .../new_delete_mismatch.cpp:3
SUMMARY: AddressSanitizer: alloc-dealloc-mismatch ... in operator delete(void*, unsigned long)
==2724==ABORTING
```

对 `int` 这种没有析构函数的类型，混用可能“碰巧能跑”，但仍然是 UB；换成有析构函数的类数组，少调用析构函数可能导致资源泄漏。ASan 会直接拦下这种不匹配。

> **记忆点：** 分配和释放看“有没有 `[]`”配对：`new[]` 只认 `delete[]`；混用是 UB，ASan 报 `alloc-dealloc-mismatch`，有析构函数的元素还会漏析构。

## 7. 易错点

1. **把 `Tracker b = a;` 当成赋值**：声明里的 `=` 是复制初始化，调用拷贝构造；只有已有对象的 `b = a;` 才调用 `operator=`。
2. **以为编译器生成的拷贝是深拷贝**：它只逐成员复制，裸指针只复制地址，两个对象共享同一块内存。
3. **以为按值返回一定发生拷贝**：C++17 返回 prvalue 保证不拷贝；返回具名局部变量才依赖 NRVO。
4. **把 NRVO 当成语言保证**：它只是可选优化，`-fno-elide-constructors` 下就会看到拷贝构造。
5. **只盯着 double-free，忘了泄漏**：隐式拷贝赋值覆盖旧指针，被赋值对象原来的内存没人释放。
6. **`new[]` 配 `delete`、`new` 配 `delete[]`**：对平凡类型可能不崩，但仍是 UB；ASan 会报 `alloc-dealloc-mismatch`。
7. **以为按值传参没有开销**：形参是新对象，至少一次拷贝构造（以及一次析构）。
8. **写了拷贝构造却忘记拷贝赋值**：两个特殊成员互相独立，只补齐一个，另一个仍会走隐式浅拷贝。

## 8. 待复习

- `Tracker b = a;`、`Tracker c(a);`、`b = a;` 分别调用哪个特殊成员？判断依据是什么？
- 按值传参、按值返回 prvalue、按值返回具名局部变量，各发生几次拷贝？哪些是保证的？
- 编译器生成的拷贝构造对裸指针做了什么？为什么会导致 double-free？
- 浅拷贝的拷贝赋值除了 double-free，还会带来什么额外问题？
- `new[]` / `delete[]` 为什么必须配对？混用时 ASan 报什么？
- 深拷贝的拷贝赋值，为什么推荐“先分配新内存，再释放旧内存”？自赋值要怎么处理？
- Rule of 3 说的是哪三个函数？为什么写了析构通常就要把拷贝也一起写？
