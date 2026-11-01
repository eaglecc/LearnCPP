# D02 任务 3：引用与指针

> 对应 [code/pointers.cpp](code/pointers.cpp)。普通构建必须零警告：
>
> ```bash
> g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror pointers.cpp -o pointers
> ./pointers
> ```
>
> 悬空指针的 ASan 演示单独构建：
>
> ```bash
> g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion \
>     -DASAN_DEMO -Wno-return-local-addr -fsanitize=address -g \
>     pointers.cpp -o pointers_asan
> ./pointers_asan
> ```
>
> `-Wno-return-local-addr` 只是为了让“故意返回局部变量地址”的演示能编译；正常代码不要用它掩盖问题。

## 今天解决的问题

- 指针和引用的根本区别：能不能为空、能不能改绑、`sizeof` 得到什么、传参时发生什么。
- 用指针遍历数组：半开区间 `[arr, arr + n)`、指针算术、原地反转、统计次数。
- `const std::vector<int>&` 与按值传参的差别：是否拷贝、能否修改、地址是否相同。
- 数组退化：为什么 `sizeof(arr)` 在 `main` 里是 20，传到函数里变成 8。
- 悬空指针：局部变量生命周期结束后继续解引用为什么是 UB；ASan 实际报了什么。

## 核心概念 & 为什么这样设计

### 1. 指针 vs 引用

| 维度 | 指针 `int*` | 引用 `int&` |
|---|---|---|
| 是否可以为空 | 可以（`nullptr`） | 不可以，必须绑定到对象 |
| 是否必须初始化 | 可以不初始化（但很危险） | 必须在定义时绑定 |
| 能否改绑到别的对象 | 可以，`p = &other` | 不可以，`r = y` 是给原对象赋值 |
| `sizeof` | 指针本身的大小（x86-64 为 8） | 被引用对象的大小（`int&` 为 4） |
| 能否做算术 | 可以（`p + 1`） | 不可以 |
| 语言层面是否独立对象 | 是，指针变量本身有地址 | 不是，引用是对象的别名 |
| 函数传参 | 显式传地址，可能为空 | 隐式传对象本身，不拷贝 |

```cpp
int x = 1;
int y = 2;

int* p = &x;
int& r = x;

std::cout << (&r == &x);     // 1（true）：r 就是 x 的别名
std::cout << sizeof(r);      // 4：被引用对象的大小
std::cout << sizeof(p);      // 8：指针本身的大小

r = y;                       // 把 y 的值赋给 x，不是让 r 改绑到 y
p = &y;                      // 指针改指向 y
```

- 引用在语言层面不是独立对象，所以没有“引用的引用”；`&r` 得到的是被引用对象的地址。
- 引用必须绑定到一块有效存储；不存在“空引用”。（`*(int*)nullptr` 这种强行构造是 UB，不是合法引用。）
- 引用一经绑定不能改绑；对引用赋值，改的是它绑定的那个对象。
- `const int&` 可以绑定字面量和临时对象，并延长临时对象的生命周期；这一点 D6/D10 再展开。

#### 函数参数怎么选

```cpp
void by_pointer(int* p);        // 可以传 nullptr；调用方要显式取地址
void by_reference(int& r);      // 不能传 nullptr；调用方直接传对象
void by_const_ref(const int& r);// 只读，不拷贝，可以接临时对象
```

- 需要“可选、可能没有”用指针（或 `std::optional`，后面再学）。
- 需要“一定有、并且要修改调用方对象”用引用。
- 只读且对象较大，用 `const T&`；小对象（`int`、指针）按值传往往更简单。

### 2. 交换：指针版与引用版

```cpp
void swap_ptr(int* a, int* b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

void swap_ref(int& a, int& b) {
    int tmp = a;
    a = b;
    b = tmp;
}
```

- 指针版：函数内拿到的是地址，必须解引用 `*a` 才能改到调用方的对象。
- 引用版：`a`、`b` 直接就是调用方对象的别名，写起来和普通变量一样。
- 两版都必须用临时变量；不要用 `a ^= b` 之类的“无临时变量交换”，可读性差，而且对同一对象自交换会出错。

运行结果：

```text
== 1. swap ==
after swap_ptr: x = 2, y = 1
after swap_ref: x = 1, y = 2
```

第一次交换后是 `(2, 1)`，第二次又换回来，说明两个版本都正确。

### 3. 指针遍历数组

#### 半开区间 `[arr, arr + n)`

```cpp
int max_ptr(const int* arr, std::size_t n) {
    if (n == 0) {
        return 0;
    }
    int max = *arr;
    for (const int* p = arr + 1; p != arr + n; ++p) {
        if (*p > max) {
            max = *p;
        }
    }
    return max;
}
```

- `arr + n` 是“最后一个元素的下一个位置”（one-past-the-end）。**可以形成**这个指针，但**不能解引用**。
- 循环条件用 `p != arr + n`，区间是左闭右开，不会漏掉最后一个元素。
- 指针算术只能在同一数组（或 one-past-the-end）内进行；跨数组比较/相减是 UB。
- `n == 0` 必须提前返回：否则 `arr + n - 1` 会变成 `arr - 1`，越界。
- `n` 是 `std::size_t`（无符号），如果写 `n - 1` 而 `n == 0`，会回绕成很大的数，这是常见 bug。

下标版实现同样的逻辑：

```cpp
int max_of(const int* arr, std::size_t n) {
    if (n == 0) {
        return 0;
    }
    int max = arr[0];
    for (std::size_t i = 1; i < n; ++i) {
        if (arr[i] > max) {
            max = arr[i];
        }
    }
    return max;
}
```

`arr[i]` 本质上就是 `*(arr + i)`；两版只是写法不同。

> 空数组返回 `0` 只是一个约定；如果数组里真的有 `0`，调用方无法区分“空数组”和“最大值是 0”。真实接口应要求非空，或返回 `std::optional<int>`（后面再学）。

#### 原地反转

```cpp
void reverse_ptr(int* arr, std::size_t n) {
    if (n < 2) {
        return;
    }
    int* left = arr;
    int* right = arr + n - 1;
    while (left < right) {
        int tmp = *left;
        *left = *right;
        *right = tmp;
        ++left;
        --right;
    }
}
```

- 两个指针从两端向中间走，交换后 `++left`、`--right`。
- `left < right` 只比较同一数组内的两个指针，合法；当元素个数为奇数时，中间元素不需要动。
- `n < 2` 提前返回，避免 `arr + n - 1` 在 `n == 0` 时越界。

引用到数组的版本：

```cpp
void reverse_ref(int (&arr)[5]) {
    for (std::size_t i = 0; i < 2; ++i) {
        int tmp = arr[i];
        arr[i] = arr[4 - i];
        arr[4 - i] = tmp;
    }
}
```

- `int (&arr)[5]` 是“引用到 `int[5]`”，括号不能省；写成 `int& arr[5]` 会变成“引用数组”，非法。
- 因为参数是引用，数组**没有退化**，函数内 `sizeof(arr) == 20`。
- 固定写 `5` 只适合演示；要接受任意长度需要模板，今天不碰。

#### 统计次数

```cpp
std::size_t count_ptr(const int* arr, std::size_t n, int value) {
    std::size_t count = 0;
    for (const int* p = arr; p != arr + n; ++p) {
        if (*p == value) {
            ++count;
        }
    }
    return count;
}

std::size_t count_ref(const std::vector<int>& v, int value) {
    std::size_t count = 0;
    for (int x : v) {
        if (x == value) {
            ++count;
        }
    }
    return count;
}
```

- 指针版遍历原始数组；引用版用 `const std::vector<int>&` 避免拷贝，再用 range-for。
- `count` 用 `std::size_t`，和元素个数/下标的类型一致，避免有符号/无符号混用。

运行结果：

```text
== 2. max ==
max_ptr = 9
max_of  = 9
== 3. reverse ==
before: 1 2 3 4 5 
after reverse_ptr: 5 4 3 2 1 
after reverse_ref: 5 4 3 2 1 
== 4. count ==
count_ptr of 1 = 2
count_ref of 1 = 2
```

### 4. const 正确性：`const T&` vs 按值

```cpp
void process_ref(const std::vector<int>& v) {
    std::cout << "process_ref:  data = " << static_cast<const void*>(v.data())
              << ", v[0] = " << v[0] << '\n';
}

void process_val(std::vector<int> v) {
    v[0] = -1;  // 改的是副本
    std::cout << "process_val:  data = " << static_cast<const void*>(v.data())
              << ", v[0] = " << v[0] << '\n';
}
```

调用：

```cpp
std::vector<int> data = {10, 20, 30};
process_ref(data);
process_val(data);
```

> 代码里没有写成同名重载 `process(...)`，因为这两个签名对左值调用会二义：
>
> ```text
> error: call of overloaded 'process(std::vector<int>&)' is ambiguous
> note: candidate: 'void process(const std::vector<int>&)'
> note: candidate: 'void process(std::vector<int>)'
> ```
>
> 所以分别命名为 `process_ref` 和 `process_val`，才能各调用一次做对比。

实际输出（地址每次运行会变，但关系不变）：

```text
== 5. const 正确性 ==
main before:  data = 0x564cd78d72f0, data[0] = 10
process_ref:  data = 0x564cd78d72f0, v[0] = 10
process_val:  data = 0x564cd78d7310, v[0] = -1
main after:   data = 0x564cd78d72f0, data[0] = 10
```

- `process_ref` 的参数是 `const std::vector<int>&`：不拷贝，`v.data()` 和调用方 `data.data()` 是同一个地址；`const` 保证函数内不能修改元素。
- `process_val` 的参数是按值：调用时拷贝整个 `vector`（包括堆上的元素缓冲区），`v.data()` 是另一个地址；函数内把 `v[0]` 改成 `-1` 只影响副本。
- 调用返回后，`main` 里的 `data[0]` 仍然是 `10`，证明按值传参没有改到原对象。
- 只读传参优先 `const T&`；确实需要在函数内修改一份副本时，才按值传（或者显式在函数内拷贝）。

`static_cast<const void*>(v.data())` 只是为了让“这里打印的是地址”这件事更明确；`int*` 直接输出时也会匹配 `operator<<(const void*)` 打印地址。显式转换能避免读者把它误认为打印元素，也避免和 `const char*` 的字符串重载混淆。这不是“硬压转换”，不改变地址值。

### 5. 数组退化：`sizeof` 为什么变小

```cpp
void show_size(const int* arr) {
    std::cout << "函数内 sizeof(arr)   = " << sizeof(arr) << "（arr 已退化为指针）\n";
}

void show_size_ref(const int (&arr)[5]) {
    std::cout << "引用数组 sizeof(arr) = " << sizeof(arr) << "（引用到数组，未退化）\n";
}

int arr[5] = {1, 2, 3, 4, 5};
std::cout << "main 里 sizeof(arr)  = " << sizeof(arr) << '\n';
show_size(arr);
show_size_ref(arr);
```

输出：

```text
== 6. 数组退化 ==
main 里 sizeof(arr)  = 20
函数内 sizeof(arr)   = 8（arr 已退化为指针）
引用数组 sizeof(arr) = 20（引用到数组，未退化）
```

- `arr` 的类型是 `int[5]`，`sizeof(arr) = 5 * 4 = 20`。
- 把数组传给 `const int*` 参数时，发生**数组到指针的退化**（array-to-pointer decay）：实际传的是 `&arr[0]`，函数内的 `arr` 是 `const int*`，所以 `sizeof(arr)` 是指针大小 8（x86-64）。
- 函数签名 `void f(const int* arr)` 里的 `arr` 从来不是数组，只是指针；写成 `void f(const int arr[])` 或 `void f(const int arr[5])` 也一样会退化，方括号里的数字会被忽略。
- `const int (&arr)[5]` 是引用到数组，不退化，所以 `sizeof(arr)` 仍是 20。代价是只能绑定长度为 5 的数组。
- 数组名不退化的场景：`sizeof(arr)`、`&arr`、`decltype(arr)`、绑定到数组引用等。其余大多数表达式里都会退化成指向首元素的指针。
- 因此函数内不能靠 `sizeof(arr) / sizeof(arr[0])` 求元素个数；必须额外传 `n`，或者用引用到数组（长度固定），或者用 `std::size(arr)`（仅当 `arr` 仍是数组类型时可用）。

### 6. 悬空指针：返回局部变量地址

```cpp
int* dangling_return() {
    int local = 42;
    return &local;  // local 在函数返回时生命周期结束
}
```

- `local` 是自动存储期对象，函数返回时生命周期结束；它占用的栈空间可能马上被别的函数调用覆盖。
- 返回的指针值本身还能打印，但它指向的对象已经不存在；解引用它是 **UB**。
- “可能不崩溃”是因为那块栈内存可能暂时还留着 `42`；但编译器可以假设 UB 不会发生，从而做任意优化。**不能依赖它读到 42，也不能依赖它崩溃。**

GCC 13.3.0 对上面这行的警告（由 `-Wall` 带出）：

```text
warning: address of local variable 'local' returned [-Wreturn-local-addr]
    return &local;
           ^~~~~~
note: declared here
    int local = 42;
```

因为普通构建要求零警告，`pointers.cpp` 把第 7 节放在 `#ifdef ASAN_DEMO` 里；普通构建根本不编译这个函数，ASan 构建再用 `-Wno-return-local-addr` 放行。

#### 实际观察：GCC 把 UB 折成了空指针

ASan 构建运行到第 7 节时：

```text
== 7. 悬空指针（ASan） ==
dangling_return() = 0（不要解引用：UB）
```

本机 g++ 13.3.0 直接把 `return &local` 优化成了 `nullptr`。这正是 UB 的可怕之处：代码看起来“应该返回一个地址”，编译器却可以因为“返回局部地址本来就是非法的”而返回 0。此时再解引用，就是空指针访问，ASan 报的是 SEGV，而不是 `stack-use-after-return`。

#### 用 global 指针把地址“带出去”，让 ASan 报清楚

```cpp
int* g_escaped = nullptr;

void escape_local() {
    int local = 42;
    g_escaped = &local;
}
```

```cpp
escape_local();
std::cout << "*g_escaped = " << *g_escaped << '\n';  // ASan 在这里报错
```

ASan 实际报告（节选，行号会随代码改动变化）：

```text
==...==ERROR: AddressSanitizer: stack-use-after-return on address 0x... at pc ...
READ of size 4 at 0x... thread T0
    #0 ... in main .../pointers.cpp:207
    ...
Address 0x... is located in stack of thread T0 at offset 32 in frame
    #0 ... in escape_local .../pointers.cpp:146

  This frame has 1 object(s):
    [32, 36) 'local' (line 147) <== Memory access at offset 32 is inside this variable
...
SUMMARY: AddressSanitizer: stack-use-after-return .../pointers.cpp:207 in main
==...==ABORTING
```

- `stack-use-after-return`：函数返回后还使用它的栈对象。
- ASan 把返回后的栈内存标记成 `f5`（见报告里的 shadow bytes），访问时立刻报错。
- 本机 g++ 13.3.0 默认检测 `stack-use-after-return`；如果换到别的环境没有报告，可以显式设置 `ASAN_OPTIONS=detect_stack_use_after_return=1` 再运行。
- 这里用全局指针只是**为了演示**，不是修复方案。正确做法是返回值本身、让调用方提供缓冲区、用 `std::vector`/`std::string` 管理生命周期，或（后面会学）智能指针。
- ASan 只检查实际执行到的路径，不是“证明程序正确”；今天只做体验，D9 系统练。

> 为什么 `dangling_return()` 的解引用没有给出 `stack-use-after-return`？
>
> 因为 GCC 已经把返回值折成 `nullptr`，真正发生的是空指针解引用。ASan 对空指针通常只报 `SEGV`。这说明：**不能假设 UB 会以某种固定方式表现**。为了让 ASan 看到“访问已返回函数的栈对象”，需要让指针值真实地逃逸出去（例如存进全局变量/输出参数）。

#### 一个实用的输出细节

ASan 报错后程序会 `abort`，标准输出如果是块缓冲，之前的 `std::cout` 内容可能还没刷出来。第 7 节在打印悬空指针后加了 `std::flush`，所以能在 ASan 报告前看到那一行。调试崩溃时，日志使用 `std::endl` 或显式 `std::flush` 也是同样的道理。

## 易错点

1. **`sizeof(指针)` 不是 `sizeof(数组)`**：函数参数里的数组已经退化成指针，`sizeof` 得到 8，不是 20。
2. **`int (&arr)[5]` 的括号不能省**：`int& arr[5]` 是“引用数组”，非法；`int (&arr)[5]` 才是“引用到数组”。
3. **`arr` 和 `&arr` 类型不同**：`arr` 退化后是 `int*`（指向首元素），`&arr` 是 `int (*)[5]`（指向整个数组）。数值上地址相同，但类型不同，指针算术步长也不同。
4. **空数组的 `n - 1` 会回绕**：`n` 是 `std::size_t`，`n == 0` 时 `n - 1` 是巨大值；先判断 `n == 0` 或 `n < 2`。
5. **指针算术只能在同一数组内**：可以形成 one-past-the-end，但不能解引用；跨数组相减/比较是 UB。
6. **返回局部变量地址是 UB**：包括返回局部数组元素地址、局部对象成员地址。函数返回后栈帧失效。
7. **悬空指针不一定崩溃**：可能读到旧值，也可能被优化成 `nullptr`，也可能崩溃；都不能依赖。
8. **`const int*` 不是 const 指针**：它指向 const int，指针本身可以改指向；`int* const` 才不能改指向。
9. **`const std::vector<int>&` 不是“传值”**：它不拷贝，函数内不能修改元素；按值传参才会拷贝，修改的是副本。
10. **`v.data()` 的地址可能变**：`vector` 扩容后旧指针/迭代器失效；今天没碰动态扩容，但这是后面常见的悬空指针来源。
11. **引用不能为空、不能改绑**：`int& r;` 非法；`r = y` 是赋值，不是改绑。
12. **ASan 不是万能的**：它只报告实际执行到的错误；没跑到的分支、未定义但未触发的行为可能漏掉。

## 待复习

- 指针与引用的区别：可空性、初始化、改绑、`sizeof`、指针算术、传参语义。
- 引用作为函数参数时，`int&`、`const int&`、按值分别适合什么场景。
- 半开区间 `[arr, arr + n)`；one-past-the-end 可以形成但不能解引用。
- `reverse_ptr` 的双指针写法；`n == 0` / `n == 1` 的边界。
- `int (&arr)[5]` 的读法，以及它为什么能阻止数组退化。
- 数组退化的规则和例外（`sizeof`、`&`、`decltype`、数组引用）。
- `process(const std::vector<int>&)` 与 `process(std::vector<int>)` 的拷贝行为、地址差异、修改是否影响调用方。
- 悬空指针的成因；为什么返回局部变量地址是 UB；为什么“可能不崩溃”不等于安全。
- GCC 的 `-Wreturn-local-addr`；ASan 的 `stack-use-after-return` 报告怎么读。
- ASan 构建为什么单独用 `#ifdef ASAN_DEMO` 和 `-Wno-return-local-addr`。
- 为什么 `dangling_return()` 在本机返回 0，而 `escape_local()` 能得到干净的 ASan 报告。
