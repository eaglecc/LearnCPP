# D05 任务：类与对象 II —— 继承与多态

> 预计 5 小时。概念 1.5h / 动手 2.5h / 笔记与 Git 1h。
> 环境：Ubuntu 24.04（WSL2 或 README 里的虚拟机），GCC 13+，CMake 3.28+，Ninja，GDB 13+，ASan。
> 构建目录统一用 `~/build/base`，源码在 `base/days/d05-inheritance-polymorphism/code/`。

D04 解决了“一个对象怎么出生、复制、死亡”。今天回答下一个问题：

> **基类指针 / 引用调用派生类函数时，运行时怎么知道该调哪一个？对象里为此多了什么？**

围绕这个问题，完成五件事：

1. 继承关系与构造 / 析构顺序；
2. 虚函数、`override` / `final`、纯虚类；
3. 虚析构与对象切片；
4. `dynamic_cast` 与 RTTI；
5. vtable 机制，用 GDB 亲眼看一次虚调用。

## 今日目标

- [ ] 说清 public 继承的含义，以及基类 / 成员 / 派生类自身的构造、析构顺序
- [ ] 能区分静态绑定和动态绑定，知道多态必须通过指针或引用
- [ ] 会用 `override` / `final`，能解释纯虚函数与抽象类
- [ ] 说清为什么多态基类的析构函数必须是 `virtual`
- [ ] 能复现并解释对象切片，知道按值传递会丢多态
- [ ] 会用 `dynamic_cast` 做安全的向下转换，知道它和 `static_cast` 的区别
- [ ] 能用 GDB 观察 vptr / vtable，并记录一次虚调用
- [ ] 完成 `Shape` 继承体系 + 多态容器，零警告
- [ ] CMake 多 target、Git 提交、笔记完成

## 今日不碰

- 多继承、虚继承、菱形继承（本阶段只做单继承）
- 模板、CRTP、concepts（D7）
- 智能指针深入（D9；今天多态容器用裸指针，D9 再换成 `unique_ptr`）
- 异常安全等级、完美转发（D10）
- 运算符重载（D6，今天只用到 `std::ostream` 的既有重载）
- 设计模式（工厂、访问者等）
- 并发（D11 之后）

> 本目录的 `code/` 已经建好；所有练习文件放进去。
> 下面的命令如果没有特别说明，默认在 `base/days/d05-inheritance-polymorphism/code/` 下执行；CMake 命令在仓库根执行。
> 今天的失败实验（`fail_*.cpp`）只用来记录错误，不进 CMake。

---

## 任务 1：继承基础与构造 / 析构顺序（约 60 min）

### 1.1 写第一组继承

在 `code/inherit_order.cpp` 里写：

```cpp
class Shape {
public:
    explicit Shape(const char* name);
    ~Shape();
    const char* name() const;
protected:
    const char* name_;
};

class Circle : public Shape {
public:
    Circle(const char* name, double radius);
    ~Circle();
    double radius() const;
private:
    double radius_;
};
```

要求：

- `Shape` 和 `Circle` 的构造 / 析构函数都打印一行，带类名和 `name_`；
- `Shape` 没有默认构造函数，`Circle` 必须在初始化列表里显式调用 `Shape(...)`；
- 在 `main` 里创建 `Circle`，观察输出。

记录完整顺序，并回答：

- 基类、成员、派生类自身的构造函数体，谁先执行？
- 析构顺序和构造顺序是什么关系？
- 如果 `Circle` 里再放一个成员对象，它的构造 / 析构排在哪里？

### 1.2 访问控制

把 `Shape::name_` 分别改成 `public` / `protected` / `private`，在 `Circle` 里访问它：

- 哪些能访问，哪些编译失败？
- 记录 `private` 版本的编译错误到 `fail_protected.cpp`（不进 CMake）。

同时说明：`class` 默认继承是 private，`struct` 默认继承是 public；显式写 `: public Shape` 才是通常想要的 is-a 关系。

### 1.3 名称隐藏

```cpp
class Base {
public:
    void foo(int);
    void foo(double);
};

class Derived : public Base {
public:
    void foo(const char*);
};
```

- `Derived d; d.foo(1);` 能不能编译？为什么？
- 在 `Derived` 里写 `using Base::foo;` 之后呢？
- 另写一组 `Base::foo` 为 `virtual` 的例子，用 `-Woverloaded-virtual` 观察“派生类同名函数隐藏基类虚函数”的警告。

记录：名称隐藏（name hiding）和重写（override）不是一回事。

### 1.4 小结表

在笔记里填：

| 问题 | 答案 |
|---|---|
| 构造顺序 | |
| 析构顺序 | |
| 基类无默认构造时怎么办 | |
| public / protected / private 继承的区别 | |
| 同名函数是隐藏还是重写 | |

---

## 任务 2：虚函数、`override` / `final`、纯虚类（约 70 min）

### 2.1 没有 `virtual` 的静态绑定

写 `code/virtual_dispatch.cpp`，先写一组**没有** `virtual` 的 `Base` / `Derived`：

```cpp
class Base {
public:
    void who() const;   // 注意：没有 virtual
};

class Derived : public Base {
public:
    void who() const;
};
```

在 `main` 里：

```cpp
Derived d;
Base* p = &d;
p->who();   // 调谁？
d.who();    // 调谁？
```

记录输出，并解释：函数的版本在**编译期**由指针 / 引用的静态类型决定，这叫静态绑定。

### 2.2 加上 `virtual` 的动态绑定

把 `Base::who` 改成 `virtual void who() const;`，派生类加 `override`：

```cpp
class Derived : public Base {
public:
    void who() const override;
};
```

再跑同样的代码，记录输出。

再验证三种调用方式：

```cpp
Base* p = &d;      // 指针：动态绑定
Base& r = d;       // 引用：动态绑定
Base b = d;        // 按值：切片，静态绑定（见任务 3）
p->who();
r.who();
b.who();
```

回答：多态生效的条件是什么？

### 2.3 `override` 与 `final`

故意写错签名：

```cpp
class Derived : public Base {
public:
    void who() override;   // Base 里是 const，这里漏了 const
};
```

用 `-Werror` 编译，记录编译错误（`fail_override.cpp`，不进 CMake）。

再做一个对照：**去掉 `override`、保留错误签名**。这时编译器不会报错，`Derived::who` 只是隐藏了 `Base::who`，多态会悄悄失效——这正是应该始终写 `override` 的原因。记录两种写法的差异。

再试：

- `final` 修饰派生类 → 不能再被继承；
- `final` 修饰虚函数 → 派生类不能重写；
- 记录对应的编译错误。

### 2.4 纯虚函数与抽象类

```cpp
class Shape {
public:
    virtual ~Shape() = default;
    virtual double area() const = 0;          // 纯虚函数
    virtual const char* name() const = 0;
};
```

- 尝试 `Shape s;` → 编译错误，记录到 `fail_abstract.cpp`；
- `Shape* p = new Circle(...);` 合法，但 `Shape` 不能直接实例化；
- 派生类只要还有一个纯虚函数没实现，就仍是抽象类。

### 2.5 构造函数 / 析构函数里的虚调用

写一个小实验：基类构造 / 析构函数里调用虚函数，派生类重写了它。

- 构造基类部分时，虚调用派发到哪个版本？
- 析构基类部分时呢？
- 解释原因：对象的派生部分此时还没构造 / 已经析构。

### 2.6 多态容器

```cpp
std::vector<Shape*> shapes;
shapes.push_back(new Circle("circle", 2.0));
shapes.push_back(new Rectangle("rect", 3.0, 4.0));

for (Shape* s : shapes) {
    std::cout << s->name() << " area=" << s->area() << '\n';
}
for (Shape* s : shapes) {
    delete s;   // D9 会换成 unique_ptr
}
```

记录输出。思考：如果 `Shape` 的析构函数不是 `virtual`，这个 `delete s` 会发生什么？（任务 3 验证）

---

## 任务 3：虚析构与对象切片（约 60 min）

### 3.1 非虚析构的后果

写 `code/virtual_dtor.cpp`：

```cpp
class Base {
public:
    Base();
    ~Base();          // 非虚
    virtual void who() const;
};

class Derived : public Base {
public:
    Derived();
    ~Derived();       // 派生类析构
};
```

```cpp
Base* p = new Derived;
delete p;             // Base 的析构函数不是 virtual
```

记录实际输出：只打印了哪几个析构？`Derived` 的析构有没有执行？

说明：通过基类指针 `delete` 一个派生类对象，如果基类析构不是 `virtual`，行为是 **UB**；在本机上通常表现为只调用 `Base::~Base`，派生类的资源没释放。这不是“泄漏一点点”，而是明确未定义。

### 3.2 加上 `virtual ~Base()`

把析构改成 `virtual ~Base();`，或者更常见的写法：

```cpp
virtual ~Base() = default;
```

再跑一次，记录析构顺序：`Derived::~Derived` → `Base::~Base`。

### 3.3 纯虚析构（可选）

```cpp
class Base {
public:
    virtual ~Base() = 0;   // 纯虚析构
};
Base::~Base() = default;   // 纯虚析构也必须有定义
```

解释：纯虚析构让类成为抽象类，但派生类析构时仍要调用基类析构，所以基类析构必须有定义。

### 3.4 对象切片

多态需要 `Base` 是具体类才能按值复制。写 `code/slicing.cpp`：

```cpp
class Animal {
public:
    virtual const char* speak() const { return "..."; }
    virtual ~Animal() = default;
};

class Dog : public Animal {
public:
    const char* speak() const override { return "woof"; }
};

void take_by_value(Animal a);      // 按值：切片
void take_by_ref(const Animal& a); // 按引用：多态

Dog d;
Animal a = d;          // 切片
take_by_value(d);      // 切片
take_by_ref(d);        // 多态
```

记录输出，并回答：

- `Animal a = d;` 之后，`a.speak()` 返回什么？为什么？
- 切片对象里有没有 `Dog` 的成员？vptr 指向哪张表？
- 为什么多态函数参数应该写 `const Base&` 或 `Base*`？

---

## 任务 4：`dynamic_cast` 与 RTTI（约 50 min）

### 4.1 指针版

写 `code/dynamic_cast.cpp`，使用任务 2 的 `Shape` / `Circle` / `Rectangle`：

```cpp
Shape* p = new Circle("circle", 2.0);

Circle* c = dynamic_cast<Circle*>(p);        // 成功
Rectangle* r = dynamic_cast<Rectangle*>(p);  // 失败，返回 nullptr
```

- 打印两个指针是否为空；
- 用 `if (Circle* c = dynamic_cast<Circle*>(p))` 的写法安全地使用结果；
- 再对 `p` 调用 `dynamic_cast<Shape*>`、`dynamic_cast<void*>`（可选），观察结果。

### 4.2 引用版

```cpp
Shape& ref = *p;
try {
    Rectangle& bad = dynamic_cast<Rectangle&>(ref);   // 失败：抛 std::bad_cast
} catch (const std::bad_cast& e) {
    std::cout << "bad_cast: " << e.what() << '\n';
}
```

记录输出。说明：指针版失败返回 `nullptr`，引用版失败抛 `std::bad_cast`。

### 4.3 `static_cast` vs `dynamic_cast`

```cpp
Circle* c1 = static_cast<Circle*>(p);    // 不做检查
Circle* c2 = dynamic_cast<Circle*>(p);   // 运行时检查
```

- 当 `p` 实际指向 `Rectangle` 时，`static_cast<Circle*>` 的结果是什么？
- 使用它的行为是什么？（UB，不要真的解引用）
- 为什么 `dynamic_cast` 需要基类至少有一个虚函数？

### 4.4 `typeid`

```cpp
std::cout << typeid(*p).name() << '\n';
std::cout << (typeid(*p) == typeid(Circle)) << '\n';
```

- 对多态类型，`typeid(*p)` 返回动态类型；
- `typeid(p)` 返回静态类型 `Shape*`；
- `typeid(...).name()` 的名字是实现定义的（GCC 下是 mangled name），不要依赖它的字符串。

### 4.5 什么时候不该用 `dynamic_cast`

如果基类已经提供了虚函数接口，优先用虚函数完成分派；`dynamic_cast` 适合“确实需要知道具体类型”的少数场景。滥用它通常说明接口设计有问题。

另外：`-fno-rtti` 会禁用 RTTI，`dynamic_cast` 和 `typeid` 都会编译失败；不要默认假设它们永远可用。

---

## 任务 5：vtable 与 GDB 观察（约 60 min）

### 5.1 先看 `sizeof`

写 `code/vtable_gdb.cpp`：

```cpp
class Empty {};

class Polymorphic {
public:
    virtual void f();
    virtual void g();
    virtual ~Polymorphic();
};

class HasData {
public:
    virtual void f();
    virtual ~HasData();
    int x;
};
```

在 `main` 里打印 `sizeof(Empty)`、`sizeof(Polymorphic)`、`sizeof(HasData)`，记录结果。

- 空类为什么是 1？
- 有虚函数的类为什么多出 8 字节（x86-64）？
- 这 8 字节是什么？

### 5.2 GDB 必做操作

用调试构建：

```bash
g++ -std=c++20 -g -O0 -Wall -Wextra -Wpedantic -Wconversion \
    vtable_gdb.cpp -o /tmp/d05_vtable
gdb /tmp/d05_vtable
```

在 GDB 里完成并记录：

```text
break Polymorphic::f
run
bt
p *obj
info vtbl obj          # GDB 8+ 可用；不行就试 info vtbl *ptr
x/8gx obj              # 看对象头部，第一个 8 字节通常是 vptr
p *(void**)&obj        # 取 vptr
p/x *(void**)&obj
```

再用一个 `Base*` 指向 `Derived` 对象：

- `p *p` 在 `set print object on` 下会不会显示动态类型？
- `info vtbl *p` 显示的是哪张表？
- 在 `Derived::who` 里打断点，用基类指针调用时会不会命中？

### 5.3 记录观察

在笔记里画一张简图：

```text
Derived 对象
┌──────────┐
│ vptr ────┼──> Derived vtable: [&Derived::who, &Derived::~Derived, ...]
├──────────┤
│ 数据成员  │
└──────────┘
```

说明：

- 每个**多态类**有一张 vtable（编译期生成，全类共享）；
- 每个**对象**有一个隐藏的 vptr（通常 8 字节），指向所属类的 vtable；
- 虚调用 = 通过 vptr 取函数地址再调用，所以是运行期决定；
- 非虚函数不进 vtable，编译期直接绑定。

### 5.4 可选：反汇编

```bash
objdump -d -C /tmp/d05_vtable | grep -A20 '<main>'
```

观察虚调用处是否出现“从对象头部取指针，再 `call *offset(%rax)`”的模式。记录一两行即可。

---

## 任务 6：`Shape` 继承体系 + 多态容器 + CMake + 笔记（约 50 min）

### 6.1 文件

```text
shape.h
shape.cpp
shape_main.cpp
```

`shape.h` 至少包含：

```cpp
#pragma once
#include <iosfwd>
#include <string>

class Shape {
public:
    virtual ~Shape() = default;
    virtual double area() const = 0;
    virtual const char* name() const = 0;
    virtual void print(std::ostream& os) const = 0;
};

class Circle : public Shape {
public:
    explicit Circle(double radius);
    double area() const override;
    const char* name() const override;
    void print(std::ostream& os) const override;
private:
    double radius_;
};

class Rectangle : public Shape {
public:
    Rectangle(double width, double height);
    double area() const override;
    const char* name() const override;
    void print(std::ostream& os) const override;
private:
    double width_;
    double height_;
};
```

要求：

- `Shape` 是抽象类，析构必须是 `virtual`；
- `area`、`name`、`print` 都是纯虚函数；
- 派生类全部使用 `override`；
- `print` 输出 `name(参数...)`，`operator<<` 可以留到 D6 再写，今天直接调用 `print(std::cout)`；
- 数值成员用 `double`，构造时拒绝负数（`throw std::invalid_argument` 可以，D10 会系统讲异常；今天只需保证非法参数不会悄悄构造成功）。

### 6.2 `shape_main.cpp`

至少验证：

```cpp
std::vector<Shape*> shapes;
shapes.push_back(new Circle(2.0));
shapes.push_back(new Rectangle(3.0, 4.0));

double total = 0.0;
for (const Shape* s : shapes) {
    s->print(std::cout);
    std::cout << " area=" << s->area() << '\n';
    total += s->area();
}
std::cout << "total=" << total << '\n';

for (Shape* s : shapes) {
    delete s;
}
```

再验证：

- 通过 `const Shape*` 调用虚函数仍然多态；
- `dynamic_cast<Circle*>` 对 `Rectangle` 对象返回 `nullptr`；
- 把 `Shape` 的析构临时改成非虚，观察 `delete` 的输出差异（记录后改回 `virtual`）。

### 6.3 CMake

在 `base/CMakeLists.txt` 中加 target（具体名称按仓库现有风格）：

| target | 源文件 |
|---|---|
| `d05-inherit` | `inherit_order.cpp` |
| `d05-virtual` | `virtual_dispatch.cpp` |
| `d05-vtable` | `vtable_gdb.cpp`（加 `-g -O0`） |
| `d05-shape` | `shape_main.cpp` + `shape.cpp` |

要求：

- C++20 + `-Wall -Wextra -Wpedantic -Wconversion`，零警告；
- `fail_override.cpp`、`fail_abstract.cpp`、`fail_protected.cpp` 不进 CMake。

```bash
cmake -S /mnt/d/Project/CPP/CPPLearn/base -B ~/build/base
cmake --build ~/build/base --target d05-inherit d05-virtual d05-vtable d05-shape -j"$(nproc)"
```

### 6.4 笔记

至少覆盖：

- 构造 / 析构顺序实测输出；
- 静态绑定 vs 动态绑定；
- `override` / `final` / 纯虚函数；
- 虚析构的实验对比（非虚 vs 虚）；
- 对象切片的输出和原因；
- `dynamic_cast` 指针版 / 引用版的差异；
- `sizeof` 变化和 vptr / vtable 图；
- GDB 观察记录；
- 3–5 条易错点。

按 `AGENTS.md` 的约定，每个知识点和实验后加 `> **记忆点：**`。

---

## 产出清单

- `code/inherit_order.cpp`、`virtual_dispatch.cpp`、`virtual_dtor.cpp`、`slicing.cpp`、`dynamic_cast.cpp`、`vtable_gdb.cpp`；
- `code/shape.h`、`shape.cpp`、`shape_main.cpp`；
- 失败实验：`fail_override.cpp`、`fail_abstract.cpp`、`fail_protected.cpp`；
- CMake 中 4 个新 target；
- `note_task*.md` 或一份 `note.md`；
- 至少一次 Git 提交，提交信息 `d05: 继承与多态`。

## 验收标准

- [ ] 能画出构造 / 析构顺序，并解释基类、成员、派生类自身的先后；
- [ ] 能说清静态绑定和动态绑定的区别，知道多态必须通过指针或引用；
- [ ] 能复现 `override` 签名写错的编译错误；
- [ ] 能解释抽象类、纯虚函数，并说明纯虚析构为什么仍需要定义；
- [ ] 能复现非虚析构 `delete` 派生类对象的问题，并说明为什么是 UB；
- [ ] 能复现对象切片，说明为什么按值传递会丢多态；
- [ ] 能区分 `static_cast` 和 `dynamic_cast`，知道引用版失败抛 `std::bad_cast`；
- [ ] 能用 GDB 看到 vptr / vtable，并解释虚调用过程；
- [ ] `Shape` 体系零警告构建，多态容器输出正确；
- [ ] 笔记完成，包含实测输出和记忆点。

## 自测问题

1. public 继承下，基类、成员对象、派生类自身的构造 / 析构顺序分别是什么？
2. 基类没有默认构造函数时，派生类构造函数必须做什么？
3. 静态绑定和动态绑定的区别是什么？多态生效需要哪些条件？
4. `override` 和 `final` 各解决什么问题？漏写 `override` 且签名写错会怎样？
5. 纯虚函数和抽象类是什么关系？抽象类能不能有数据成员和普通成员函数？
6. 为什么多态基类的析构函数必须是 `virtual`？如果析构函数非虚，`delete base_ptr` 会怎样？
7. 什么是对象切片？为什么按值传递基类会丢多态？
8. `dynamic_cast<Derived*>` 失败返回什么？引用版失败呢？
9. `static_cast` 和 `dynamic_cast` 在向下转换时有什么区别？
10. 多态类的对象里，vptr 和 vtable 分别是什么？`sizeof` 为什么多 8 字节？
11. 构造函数 / 析构函数里调用虚函数，会派发到哪个版本？为什么？
12. 虚调用在汇编层面大致是什么样子的？

## 完成后

告诉我「D05 完成」，并附上：

- `cmake --build` 的完整输出（确认零警告）；
- 构造 / 析构顺序、切片、虚析构三组实测输出；
- GDB 中 `info vtbl` / `x/8gx` 的记录；
- `note_task*.md` 或 `note.md`；
- `git log --oneline --graph`。

我会检查 `Shape` 体系的虚析构、多态容器释放是否正确，以及你是否真的在 GDB 里看到了 vtable；确认后再进入 D06（运算符重载与现代 C++）。
