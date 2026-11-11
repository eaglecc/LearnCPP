# D04 任务 3：Rule of 0/3/5 与手写 `String`

> 环境：WSL2 Ubuntu 24.04，g++ 13.3.0。
> 代码目录：[code/](code/)
> 本节的输出都来自实际编译、运行；`String` 和 `Person` 都通过了 ASan + UBSan，退出码 0、无泄漏。

## 0. 速查

- **Rule of 0**：成员自己管资源（`std::string`、`std::vector`），不写析构 / 拷贝 / 移动，让编译器生成。
- **Rule of 3**：手写析构、拷贝构造、拷贝赋值中的任何一个，通常三个都要写。
- **Rule of 5**：Rule of 3 再加移动构造、移动赋值。
- 裸 `char*` + `new[]` 的类属于 Rule of 3：析构 `delete[]`，拷贝必须**深拷贝**。
- 拷贝赋值三件套：判自赋值、先分配后释放、返回 `*this`。
- 写了析构 / 拷贝后，编译器**不会**再隐式生成移动成员，右值会退化成 `const&` 拷贝。
- 移动构造常标 `noexcept`，因为 `std::vector` 扩容时只对 `noexcept` 的移动做移动，否则退回复制。

## 1. 文件清单

| 文件 | 内容 | 结果 |
|---|---|---|
| `my_string.h` | `String` 接口：`data_` + `size_` | — |
| `my_string.cpp` | 默认 / 带参 / 拷贝构造、拷贝赋值、析构 | 零警告 |
| `string_main.cpp` | 深拷贝、自赋值、链式赋值、源对象销毁后拷贝仍有效 | 运行通过，ASan/UBSan 干净 |
| `rule_of_zero.cpp` | `Person` 持有 `std::string`，不写任何特殊成员 | 运行通过，ASan/UBSan 干净 |

Rule of 5 的移动成员今天只解释、不实现，所以没有对应代码文件。

## 2. 构建与运行

```bash
cd base/days/d04-classes-lifecycle/code

g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    string_main.cpp my_string.cpp -o /tmp/d04_string
/tmp/d04_string

g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    string_main.cpp my_string.cpp -o /tmp/string_asan
/tmp/string_asan

g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -g -fsanitize=address,undefined -fno-omit-frame-pointer \
    rule_of_zero.cpp -o /tmp/d04_rule_of_zero
/tmp/d04_rule_of_zero
```

> **记忆点：** 手写资源类必须用 ASan 跑测试；“能跑”不等于“没有 double-free / 泄漏”，只是错误还没被触发。

## 3. 先判断：这个类需要哪些特殊成员

### 3.1 对照表

| 类的成员 | 需要手写析构？ | 需要手写拷贝？ | 需要手写移动？ | 属于 Rule of ? |
|---|---|---|---|---|
| `int`、`double` 等值类型 | 否 | 否 | 否 | Rule of 0 |
| `std::string`、`std::vector` | 否 | 否 | 否 | Rule of 0 |
| 裸 `char*` + `new[]` | **是**（`delete[]`） | **是**（深拷贝） | 建议写（偷指针） | Rule of 3 → 5 |
| `FILE*` / 文件描述符 | **是**（`fclose` / `close`） | 通常**禁止**（`= delete`） | 可以写（转移所有权） | Rule of 5，常按 move-only |

判断方法：**看成员“拷贝”之后会不会出现两个对象共享同一份资源**。

- `int` 复制的是值，互不影响 → 编译器生成的版本就正确；
- `std::string` 自己实现了深拷贝，复制后各自拥有内存 → 外层类不用管；
- 裸 `char*` 复制的是地址，两个对象指向同一块内存 → 必须自己写深拷贝和释放；
- `FILE*` 复制的是文件句柄，两个对象会关闭同一个文件 → 通常直接禁止拷贝。

### 3.2 Rule of 0/3/5

- **Rule of 0**：类不直接拥有资源，成员（如 `std::string`）各自遵守 Rule of 3/5，外层什么都不用写。
- **Rule of 3**：只要需要手写析构、拷贝构造、拷贝赋值中的**任意一个**，另外两个通常也需要。三者对应“释放、复制、覆盖”三种资源操作，缺一个就会漏水或 double-free。
- **Rule of 5**：Rule of 3 + 移动构造 + 移动赋值，用“转移所有权”代替“复制”，提高性能。

> **记忆点：** 判断依据不是“类大不大”，而是“拷贝后是否有两个对象共享同一份资源”；共享就手写 Rule of 3，想优化再加 Rule of 5。

## 4. 手写 `String`

### 4.1 设计

```cpp
class String {
public:
    String();
    explicit String(const char* str);
    String(const String& other);
    String& operator=(const String& other);
    ~String();

    const char* c_str() const;
    std::size_t size() const;
    bool empty() const;

private:
    char* data_;
    std::size_t size_;   // 不含结尾的 '\0'
};
```

不变量：`data_` 始终指向一块 `size_ + 1` 字节的内存，`data_[size_] == '\0'`。所有成员函数都必须维持这个不变量。

### 4.2 构造函数

```cpp
String::String() : data_(new char[1]{'\0'}), size_(0) {}

String::String(const char* str)
    : data_(allocate_and_copy(str, std::strlen(str))),
      size_(std::strlen(str)) {}

String::String(const String& other)
    : data_(allocate_and_copy(other.data_, other.size_)),
      size_(other.size_) {}
```

其中 `allocate_and_copy` 是文件内的辅助函数：

```cpp
char* allocate_and_copy(const char* src, std::size_t size) {
    char* data = new char[size + 1];
    std::memcpy(data, src, size);
    data[size] = '\0';
    return data;
}
```

- 默认构造也分配 1 字节放 `'\0'`，保证 `c_str()` 永远返回合法空串，而不是 `nullptr`；
- 带参构造用 `strlen` 求长度，分配 `size + 1`，复制内容再补 `'\0'`；
- 拷贝构造**重新分配**一块内存再复制，这就是深拷贝。两个 `String` 的 `data_` 地址不同。

> **记忆点：** 默认构造也要让 `c_str()` 合法；拷贝构造必须 `new[]` 一块新内存，不能只复制指针。

### 4.3 拷贝赋值

```cpp
String& String::operator=(const String& other) {
    if (this == &other) {
        return *this;
    }

    char* new_data = allocate_and_copy(other.data_, other.size_);
    delete[] data_;
    data_ = new_data;
    size_ = other.size_;
    return *this;
}
```

三个要点：

1. **先判自赋值**：`c = c` 时如果不判，先 `delete[] data_` 再读 `other.data_`，而 `other` 就是自己，读到的是已释放内存（use-after-free）。
2. **先分配后释放**：如果先 `delete[] data_` 再 `new`，一旦 `new` 抛 `std::bad_alloc`，旧数据已经没了，对象还处于坏状态；先分配成功再释放旧内存，分配失败时原对象原封不动。
3. **返回 `*this`**：赋值运算符返回引用，`d = b = a` 才能从右往左链式执行。

> **记忆点：** 拷贝赋值口诀：自赋值先返回，新内存先分配，旧内存后释放，最后返回 `*this`。

### 4.4 析构与观察函数

```cpp
String::~String() { delete[] data_; }
const char* String::c_str() const { return data_; }
std::size_t String::size() const { return size_; }
bool String::empty() const { return size_ == 0; }
```

`new[]` 必须配 `delete[]`；即使 `data_` 为空串，它也指向 1 字节的合法内存，所以不需要判空。

> **记忆点：** 谁申请谁释放：构造函数 `new char[]`，析构函数就 `delete[]`，一对一对出现。

## 5. 测试结果

`string_main.cpp` 覆盖了任务要求的全部场景：

```cpp
String a("hello");
String b = a;             // 拷贝构造
String c;                 // 默认构造
c = a;                    // 拷贝赋值
c = c;                    // 自赋值
String d("x");
d = b = a;                // 链式赋值
```

实际输出（ASan + UBSan 构建）：

```text
== 基本内容 ==
a: hello (size=5)
b: hello (size=5)
c: '' (size=0, empty=true)

== 深拷贝：地址必须不同 ==
a.data_ = 0x502000000010
b.data_ = 0x502000000030

== 拷贝赋值 ==
c = a  -> hello (size=5)
c = c  -> hello (size=5)

== 链式赋值 ==
d = b = a -> d=hello, b=hello

== 源对象销毁后，拷贝仍然有效 ==
copy = temporary
```

逐项对应：

| 现象 | 说明 |
|---|---|
| `a.data_ != b.data_` | 拷贝构造是深拷贝，两块独立内存 |
| `c = c` 不崩、内容仍为 `hello` | 自赋值检查生效，没有读到已释放内存 |
| `d = b = a` 后 `d`、`b` 都是 `hello` | `operator=` 返回 `*this`，链式赋值成立 |
| `temp` 销毁后 `copy` 仍输出 `temporary` | `copy` 拥有自己那块内存，不依赖源对象 |

ASan + UBSan 没有任何额外报错、退出码 0，说明没有越界、double-free 或泄漏。

> **记忆点：** 深拷贝的验收标准是“内容相同、地址不同、源对象销毁后拷贝仍有效”；ASan 无输出才算真正过关。

## 6. Rule of 0 对照：`Person`

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

Person a(long_name, 30);
Person b = a;   // 编译器生成的拷贝构造
```

`std::string` 自己遵守 Rule of 3/5，会正确完成深拷贝；`Person` 不需要写析构、拷贝、移动。实际输出（ASan + UBSan）：

```text
a: size=64, age=30
b: size=64, age=30
内容相同: true
地址不同: true
a.data=0x507000000100, b.data=0x507000000170
```

`long_name` 用 64 个字符，超过 libstdc++ 的 SSO 容量，确保 `std::string` 真的在堆上分配；拷贝后地址不同，证明深拷贝由 `std::string` 完成。

> **记忆点：** 成员已经遵守 Rule of 0/3/5 时，外层类什么都不用写；这就是“优先用会管资源的成员，而不是裸指针”的原因。

## 7. Rule of 5：只解释，不实现

五个特殊成员：

```cpp
~String();
String(const String&);                 // 拷贝构造
String& operator=(const String&);      // 拷贝赋值
String(String&&) noexcept;             // 移动构造（D10）
String& operator=(String&&) noexcept;  // 移动赋值（D10）
```

### 7.1 为什么写了析构和拷贝后，编译器不会生成移动成员

标准规定：只要用户声明了**拷贝构造、拷贝赋值、移动赋值、析构**中的任意一个，编译器就**不隐式声明**移动构造和移动赋值。`String` 同时写了析构、拷贝构造、拷贝赋值，所以它没有移动构造。

实测 `std::move(a)` 的效果：

```cpp
String a("abc");
String b = std::move(a);

std::cout << "a = " << a.c_str() << "  addr=" << static_cast<const void*>(a.c_str()) << '\n';
std::cout << "b = " << b.c_str() << "  addr=" << static_cast<const void*>(b.c_str()) << '\n';
```

```text
a = abc  addr=0x60b84fe012b0
b = abc  addr=0x60b84fe012d0
is_move_constructible      = true
is_nothrow_move_constructible = false
```

`std::move(a)` 只是把 `a` 转成右值，没有移动构造时右值会绑定到 `const String&`，调用**拷贝构造**。所以 `a` 内容仍在、地址不同。注意 `is_move_constructible_v<String>` 为 `true` 具有误导性：拷贝构造也能接受右值，并不代表真的存在移动构造；`is_nothrow_move_constructible_v` 才是 `false`。

### 7.2 移动构造通常做什么

```cpp
String::String(String&& other) noexcept
    : data_(other.data_), size_(other.size_) {
    other.data_ = nullptr;   // 关键：源对象放弃所有权
    other.size_ = 0;
}
```

- “偷”走源对象的指针和长度，不重新分配、不复制内容；
- 必须把源对象置空，否则源对象析构时会 `delete[]` 已经被偷走的指针 → double-free；
- 源对象之后处于“有效但为空”的状态，仍可安全析构或赋值。

### 7.3 为什么移动构造常标 `noexcept`

`std::vector` 扩容时要把旧元素搬到新内存。为了提供强异常保证，它只在移动构造是 `noexcept` 时才移动，否则退回复制。实测：

```cpp
struct Probe {
    int id;
    explicit Probe(int i) : id(i) {}
    Probe(const Probe& o) : id(o.id) {
        std::cout << "copy " << id << '\n';
    }
    Probe(Probe&& o) noexcept : id(o.id) {   // 对照版本去掉 noexcept
        std::cout << "move " << id << '\n';
    }
};

std::vector<Probe> v;
v.reserve(1);
for (int i = 0; i < 3; ++i) v.emplace_back(i);
```

```text
--- noexcept move ---
move 0
move 0
move 1
```

去掉 `noexcept` 再跑：

```text
--- throwing move ---
copy 0
copy 0
copy 1
```

扩容时三次搬移，`noexcept` 版本全部用移动，非 `noexcept` 版本全部退化成拷贝。

### 7.4 为什么今天不实现移动也能通过所有测试

`String b = std::move(a);` 会退化成拷贝构造，`return` 临时对象也走拷贝（或 C++17 的保证消除），程序仍然正确，只是多一次分配和复制。D10 实现移动构造 / 移动赋值后，这些场景才会变成“偷指针”，性能上的差别才会体现出来。

> **记忆点：** 移动构造 = 偷指针 + 把源对象置空；标 `noexcept` 是为了让 `vector` 扩容时敢用移动，否则它宁可不移动、退回复制。

## 8. 易错点

1. **拷贝赋值不判自赋值**：`c = c` 时先释放自己的内存，再读 `other.data_`（就是自己）→ use-after-free。
2. **拷贝赋值先释放后分配**：`new` 抛异常时旧数据已丢，对象无法恢复；应先分配成功再释放旧内存。
3. **赋值运算符返回 `void`**：`d = b = a` 无法编译，链式赋值失效。
4. **`size_` 把结尾 `'\0'` 算进去**：`size()` 偏大 1，`allocate_and_copy` 和边界判断都会错。
5. **默认构造不分配内存**：`c_str()` 返回 `nullptr`，`strlen` / `std::cout` 使用它可能崩溃。
6. **拷贝构造只复制指针**：又回到浅拷贝，两个对象 double-free。
7. **`new[]` 配 `delete`**：ASan 报 `alloc-dealloc-mismatch`，是 UB。
8. **以为 `is_move_constructible_v<T>` 为 true 就代表有移动构造**：拷贝构造也能绑定右值，要配合 `is_nothrow_move_constructible` 和实际行为判断。
9. **移动构造不把源对象置空**：源对象析构时 `delete[]` 同一个指针 → double-free。
10. **移动构造不标 `noexcept`**：`std::vector` 扩容退化成拷贝，移动优化白写。

## 9. 待复习

- 你的类需要 Rule of 0、Rule of 3 还是 Rule of 5？判断依据是什么？
- 拷贝赋值为什么要先判自赋值？`c = c` 不判会发生什么？
- 为什么拷贝赋值要“先分配新内存，再释放旧内存”？
- `String` 的 `size_` 为什么不包含 `'\0'`？默认构造为什么要分配 1 字节？
- 写了析构和拷贝后，为什么编译器不再生成移动构造？`std::move` 在这种情况下调用什么？
- 移动构造要做什么？为什么必须把源对象的指针置空？
- 移动构造为什么常标 `noexcept`？不标会对 `std::vector` 产生什么影响？
- 为什么 `Person` 里放 `std::string` 就不用写任何特殊成员？
