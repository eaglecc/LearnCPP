# D05 任务 1：继承基础与构造 / 析构顺序

> 环境：WSL2 Ubuntu 24.04，g++ 13.3.0。
> 代码目录：[code/](code/)
> 下面的输出和报错均来自实际编译、运行；失败实验只用来记录错误，不进 CMake。

## 0. 速查

- 构造顺序：**基类 → 派生类成员 → 派生类构造函数体**；析构顺序完全反过来。
- 基类没有默认构造函数时，派生类必须在初始化列表里显式调用 `Shape(...)`。
- 成员访问：`public` 派生类和类外都能用；`protected` 只有自己和派生类能用；`private` 只有自己能用。
- `class` 默认 private 继承，`struct` 默认 public 继承；通常要显式写 `: public Shape`。
- 派生类同名函数会**隐藏**基类所有同名重载，不是重载；要合并重载集就写 `using Base::foo;`。
- 隐藏（name hiding）和重写（override）不是一回事：隐藏只看名字，重写要求基类 `virtual` 且签名一致。

> **记忆点：** 继承的构造像“先打地基再盖楼”，析构像“先拆楼再拆地基”；名字一遮就是一整片，`using` 才能把基类重载重新请回来。

## 2. 构建与运行

```bash
cd base/days/d05-inheritance-polymorphism/code
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    inherit_order.cpp -o /tmp/d05_inherit
/tmp/d05_inherit
```

实际输出：

```text
Shape constructor called for MyCircle
Circle constructor called for MyCircle with radius 5
Circle name: Shape::name() called
MyCircle
Circle radius: Circle::radius() called
5
Circle destructor called for Shape::name() called
MyCircle
Shape destructor called for MyCircle
```

带 `-Werror` 仍然零警告。

> **记忆点：** 编译顺序正确不代表行为正确；构造 / 析构实验必须看**打印顺序**，光看“能跑”发现不了顺序问题。

## 3. 构造 / 析构顺序（任务 1.1）

### 3.1 代码

```cpp
class Shape {
public:
    explicit Shape(const char* name) : name_(name) {
        std::cout << "Shape constructor called for " << name << std::endl;
    }
    ~Shape() {
        std::cout << "Shape destructor called for " << name_ << std::endl;
    }
    const char* name() const {
        std::cout << "Shape::name() called" << std::endl;
        return name_;
    }
private:
    const char* name_;
};

class Circle : public Shape {
public:
    Circle(const char* name, double radius) : Shape(name), radius_(radius) {
        std::cout << "Circle constructor called for " << name
                  << " with radius " << radius << std::endl;
    }
    ~Circle() {
        std::cout << "Circle destructor called for " << name() << std::endl;
    }
    double radius() const { /* ... */ }
private:
    double radius_;
};
```

`main` 里只创建一个局部对象：

```cpp
Circle c("MyCircle", 5.0);
```

### 3.2 输出分解

```text
Shape constructor called for MyCircle      ← 基类构造
Circle constructor called for MyCircle ... ← 派生类构造函数体
...
Circle destructor called for ...           ← 派生类析构函数体
Shape destructor called for MyCircle       ← 基类析构
```

- 构造时先执行 `Shape` 的构造函数（包括它的初始化列表和函数体），再执行 `Circle` 的初始化列表 / 函数体。
- 析构时顺序反过来：先执行 `Circle::~Circle`，再执行 `Shape::~Shape`。
- `Circle::~Circle` 里调用 `name()`，实际进入的是 `Shape::name()`（非虚函数，静态绑定），所以多打印了一行 `Shape::name() called`。此时 `Circle` 部分正在析构，但基类部分仍然有效，读 `name_` 是安全的。
- 析构顺序与析构函数体里“用了什么”无关；它由对象层次决定。

> **记忆点：** 析构顺序永远是构造的严格逆序：基类先出生、最后死亡；派生类后出生、最先死亡。

### 3.3 基类没有默认构造函数

`Shape` 只提供了 `explicit Shape(const char*)`，没有 `Shape()`。`Circle` 的构造函数如果不在初始化列表里调用 `Shape(name)`，编译失败：

```cpp
struct Shape { explicit Shape(const char*) {} };
struct Circle : Shape { Circle() {} };   // 没有初始化基类
```

实际报错：

```text
error: no matching function for call to ‘Shape::Shape()’
note: candidate: ‘Shape::Shape(const char*)’
note:   candidate expects 1 argument, 0 provided
```

编译器会先尝试用“默认构造”初始化基类；基类没有默认构造，就只能由派生类显式传参。所以写 `Circle(...) : Shape(name), radius_(radius)` 是必须的，不是风格问题。

> **记忆点：** 派生类构造函数的初始化列表可以省略基类初始化，前提是基类有默认构造；省不了的时候，编译器只会报“no matching function for call to Shape::Shape()”。

## 4. 派生类里的成员对象（任务 1.1 第三问）

`member_order.cpp` 在 `Circle` 里再加一个成员 `Member tag_`：

```cpp
class Circle : public Shape {
public:
    Circle(const char* name, double radius) : Shape(name), tag_("circle.tag") {
        std::cout << "Circle ctor body: " << name << ", r=" << radius << '\n';
    }
    ~Circle() { std::cout << "Circle dtor body\n"; }
private:
    Member tag_;
};
```

实际输出：

```text
Shape ctor: C
Member ctor: circle.tag
Circle ctor body: C, r=2
Circle dtor body
Member dtor: circle.tag
Shape dtor: C
```

完整顺序是：

| 阶段  | 顺序                                   |
| --- | ------------------------------------ |
| 构造  | 基类 `Shape` → 派生类成员 `tag_` → 派生类构造函数体 |
| 析构  | 派生类析构函数体 → 派生类成员 `tag_` → 基类 `Shape` |

成员对象夹在“基类”和“派生类自身”之间：构造时晚于基类、早于派生类函数体；析构时早于基类、晚于派生类函数体。多个成员之间仍按声明顺序构造、逆序析构（D04 已实测）。

> **记忆点：** 一个对象的完整构造顺序是“基类 → 自己的成员 → 自己的函数体”；成员永远不属于“派生类自身”那一层。

## 5. 访问控制（任务 1.2）

### 5.1 三种成员访问级别

`access_control.cpp` 里 `Shape::name_` 为 `protected`，`Circle::peek()` 可以访问；类外只能通过 `name()` 访问。

```cpp
class Circle : public Shape {
public:
    const char* peek() const { return name_; }  // protected：派生类可以访问
};
```

实际输出：

```text
Circle::peek() = protected-circle
Shape::name() = protected-circle
OpenCircle::peek() = public-circle
outside access = public-circle
```

| 基类成员        | 派生类内部 | 类外通过派生类对象 |
| ----------- | ----- | --------- |
| `public`    | ✅     | ✅         |
| `protected` | ✅     | ❌         |
| `private`   | ❌     | ❌         |

`private` 成员也会被继承（对象里有这块存储），只是派生类的代码看不到它。`protected` 是“给派生类留的门”，不是“给外部留的门”。

> **记忆点：** `protected` 的边界是派生类，不是类外；`private` 连派生类都进不去。

### 5.2 实测报错

`fail_protected.cpp` 同时演示两个错误：

```cpp
class ClosedCircle : public ClosedShape {
public:
    const char* peek() const { return name_; }  // ClosedShape::name_ 是 private
};

int main() {
    Circle c("circle", 1.0);
    std::cout << c.name_ << '\n';               // Shape::name_ 是 protected
}
```

实际报错：

```text
fail_protected.cpp:35:39: error: ‘const char* ClosedShape::name_’ is private within this context
   35 |     const char* peek() const { return name_; }
      |                                       ^~~~~
fail_protected.cpp:40:20: error: ‘const char* Shape::name_’ is protected within this context
   40 |     std::cout << c.name_ << '\n';
      |                    ^~~~~
note: field ‘const char* Shape::name_’ can be accessed via ‘const char* Shape::name() const’
```

两条错误正好对应两条边界：`private` 挡派生类，`protected` 挡类外。

> **记忆点：** 报错里的 `is private within this context` / `is protected within this context` 就是访问级别在说“门在哪里”；看到它先判断当前代码在类内、派生类内还是类外。

### 5.3 继承方式：`class` 默认 private，`struct` 默认 public

同一个基类，不写继承方式时：

```cpp
class B { public: void f() {} };
class D : B {};        // class 默认 private 继承
int main() { D d; d.f(); }
```

实际报错：

```text
error: ‘void B::f()’ is inaccessible within this context
note: declared here
error: ‘B’ is not an accessible base of ‘D’
```

把同样的代码改成 `struct`：

```cpp
struct B { void f() {} };
struct D : B {};       // struct 默认 public 继承
int main() { D d; d.f(); }
```

编译通过、零警告。所以通常显式写 `class Circle : public Shape`，让“派生类对象可以被当成基类对象使用”这件事成立。

需要区分两个位置上的访问控制：

- 成员声明处的 `public` / `protected` / `private`：控制**谁能访问这个成员**。
- 继承列表里的 `public` / `protected` / `private`：控制**基类成员经过这次继承后，对派生类的使用者来说是什么级别**。

public 继承保留基类的访问级别，也是最常见的 is-a 关系。

> **记忆点：** 成员访问级别决定“谁能碰”，继承方式决定“继承过来后对外还算什么级别”；`class` 默认 private 继承是 C++ 最爱埋的坑。

> **记忆点：** 继承方式只改变“继承过来的成员对外的访问级别”：private 继承把 public/protected 都降成 private；而基类原本的 private 成员，任何派生类都碰不到。


## 6. 名称隐藏（任务 1.3）

### 6.1 同名函数会隐藏整个重载集

`fail_name_hiding.cpp`：

```cpp
class Base {
public:
    void foo(int x);
    void foo(double x);
};

class Derived : public Base {
public:
    void foo(const char* s);
};

Derived d;
d.foo("hello");  // OK：Derived::foo
d.foo(42);       // 编译失败
```

实际报错：

```text
fail_name_hiding.cpp:18:11: error: invalid conversion from ‘int’ to ‘const char*’ [-fpermissive]
   18 |     d.foo(42);
      |           ^~
note:   initializing argument 1 of ‘void Derived::foo(const char*)’
   12 |     void foo(const char* s) { ... }
```

`Derived` 作用域里一旦出现 `foo`，编译器就不再往基类作用域找其他 `foo` 重载。`Base::foo(int)`、`Base::foo(double)` 全部被隐藏，只剩 `Derived::foo(const char*)` 参与重载解析，`42` 转不成指针，所以报错。

### 6.2 用 `using` 把基类重载请回来

`name_hiding.cpp` 加一行：

```cpp
class Derived : public Base {
public:
    using Base::foo;
    void foo(const char* s);
};
```

实际输出：

```text
Derived::foo(const char*) hello
Base::foo(int) 42
Base::foo(double) 3.14
```

`using Base::foo;` 把基类的所有 `foo` 重载引入派生类作用域，和 `Derived::foo` 组成同一个重载集，再按实参选最匹配的版本。

> **记忆点：** 派生类只要声明了同名函数，基类那一整组同名函数就被遮住；`using Base::foo;` 是把整组重新拉进重载集的唯一常用手段。

### 6.3 隐藏 vs 重写：`-Woverloaded-virtual`

`virtual_hiding.cpp`：

```cpp
class Base {
public:
    virtual void foo(int x);
    virtual ~Base() = default;
};

class Derived : public Base {
public:
    void foo(double x);   // 隐藏 Base::foo(int)，不是重写
};
```

用 `-Woverloaded-virtual` 编译（注意它**不在** `-Wall` 里，要显式加）：

```bash
g++ -std=c++20 -Wall -Wextra -Woverloaded-virtual -c virtual_hiding.cpp
```

实际警告：

```text
virtual_hiding.cpp:4:18: warning: ‘virtual void Base::foo(int)’ was hidden [-Woverloaded-virtual=]
    4 |     virtual void foo(int x);
      |                  ^~~
virtual_hiding.cpp:10:10: note:   by ‘void Derived::foo(double)’
   10 |     void foo(double x);
```

这里 `Derived::foo(double)` 参数类型不同，没有重写 `Base::foo(int)`，只是把名字遮住了。通过 `Base*` 调用 `foo(1)` 仍会进入 `Base::foo(int)`。如果本意是重写，签名必须完全一致，并且应该写 `override` 让编译器检查（任务 2.3 会实测漏写 `const` 的情况）。

|         | 名称隐藏                             | 重写（override）           |
| ------- | -------------------------------- | ---------------------- |
| 触发条件    | 派生类出现同名函数                        | 基类函数 `virtual`，派生类签名一致 |
| 影响范围    | 基类所有同名重载都不可见                     | 只替换对应的那个虚函数            |
| 编译器检查   | 默认不报错；`-Woverloaded-virtual` 可提示 | 写 `override` 后签名不符直接报错 |
| 恢复 / 合并 | `using Base::foo;`               | 不需要                    |

> **记忆点：** 同名不等于重写：隐藏看“名字和所在作用域”，重写看“virtual + 签名一致”；漏写 `override` 时，写错的签名会静默退化成隐藏。

## 7. 小结表（任务 1.4）

| 问题                                 | 答案                                                                              |
| ---------------------------------- | ------------------------------------------------------------------------------- |
| 构造顺序                               | 基类构造 → 派生类成员构造 → 派生类构造函数体                                                       |
| 析构顺序                               | 派生类析构函数体 → 派生类成员析构 → 基类析构（与构造严格逆序）                                              |
| 基类无默认构造时怎么办                        | 在派生类构造函数的初始化列表里显式调用 `Base(args...)`                                             |
| public / protected / private 继承的区别 | 继承方式降低（或不降低）基类成员在派生类中的可见级别；`class` 默认 private，`struct` 默认 public，通常显式写 `public` |
| 同名函数是隐藏还是重写                        | 默认是隐藏；只有基类 `virtual` 且签名一致（参数、`const` 等）才是重写，写 `override` 让编译器把关                |

> **记忆点：** 这张表背下来的顺序是“基类先、成员其次、自己最后；析构倒着来；名字会遮，签名对了才叫重写”。

## 8. 易错点

1. **以为析构顺序和构造顺序一样**：完全相反；构造基类先，析构基类最后。
2. **以为成员对象先于基类构造**：基类先于派生类成员；成员只晚于基类、早于派生类函数体。
3. **以为 `private` 成员没有被继承**：存储和继承关系都在，只是派生类代码不能访问。
4. **把 `protected` 当成“类外也能用”**：类外访问 `protected` 成员会报 `is protected within this context`。
5. **以为 `class D : B` 默认是 public 继承**：不是，默认 private；`struct` 才是默认 public。
6. **以为派生类同名函数会和基类构成重载**：会隐藏基类整个同名重载集，必须 `using Base::foo;`。
7. **以为参数不同也算重写**：参数不同只是隐藏；重写要求签名一致，最好写 `override` 让编译器检查。
8. **以为 `-Wall` 会提示虚函数被隐藏**：不会；需要显式加 `-Woverloaded-virtual`。
9. **在派生类析构函数里调用非虚的基类函数**：本身安全，但不要误以为会派发到派生类版本；析构阶段的动态类型问题在任务 2.5 专门实验。

## 9. 待复习

- 一个 `Derived` 对象的构造 / 析构完整顺序是什么？成员对象排在哪一步？
- 基类没有默认构造时，派生类构造函数必须怎么写？不写报什么错？
- `public` / `protected` / `private` 成员分别对派生类和类外可见吗？
- `class` 和 `struct` 的默认继承方式分别是什么？
- 派生类声明同名函数后，基类的其他重载还能被调用吗？怎么恢复？
- 名称隐藏和重写分别需要什么条件？漏写 `override` 的危险在哪里？
- `-Woverloaded-virtual` 为什么默认不开？它提示的是隐藏还是重写失败？
