# D03 任务：函数、链接、GDB 入门

> 预计 5 小时。概念 1h / 动手 3h / 笔记与 Git 1h。
> 环境：Ubuntu 24.04（WSL2 或 README 里的虚拟机），GCC 13+，CMake 3.28+，Ninja，GDB。
> 构建目录统一用 `~/build/base`，源码在 `base/days/d03-functions-gdb/code/`。

今天的核心是三件事：

1. **函数怎么被选中**：重载解析、默认参数；
2. **符号怎么跨翻译单元（TU）解析**：命名空间、链接属性、声明 vs 定义；
3. **程序怎么在 GDB 里停下来**：断点、单步、调用栈、watchpoint。

## 今日目标

- [ ] 能说清重载解析的优先级：精确匹配 > 提升 > 转换，并能复现歧义
- [ ] 掌握默认参数“从右往左”的规则，以及与重载组合时的歧义
- [ ] 分清命名空间、`using`、`static`、匿名 namespace 的链接属性
- [ ] 能独立组织多文件工程，区分编译错误和链接错误
- [ ] 用 GDB 完成一次断点、单步、`bt`、`frame`、`watch` 调试并修复 bug
- [ ] 完成 Git 分支、合并、冲突处理
- [ ] CMake 多 target、零警告，`note.md` 完成

## 今日不碰

- 自己定义 `class` / `struct`（D4 开始）；标准库容器可以当黑盒用
- 动态内存 `new/delete`、智能指针
- 模板、lambda、异常、多线程
- 继承、多态、虚函数
- 移动语义深入（`std::move` / `std::forward` 只认识名字）

> 本目录的 `code/` 已经建好；所有练习文件放进去。
> 下面的命令如果没有特别说明，默认在 `base/days/d03-functions-gdb/code/` 下执行；CMake 命令在仓库根执行。

---

## 任务 1：重载解析与默认参数（约 60 min）

### 1.1 精确匹配 / 提升 / 转换

在 `code/overload.cpp` 里写第一组：

```cpp
void pick(int);
void pick(double);
```

分别调用：

```cpp
pick('a');
pick(1.0f);
pick(1);
pick(1.0);
```

先预测，再运行验证。在笔记里填一张表：

| 实参 | 可行候选 | 选择的函数 | 理由 |
|---|---|---|---|
| `'a'` |  |  |  |
| `1.0f` |  |  |  |
| `1` |  |  |  |
| `1.0` |  |  |  |

提示：`char → int` 是整型提升（promotion），`char → double` 是转换（conversion）；`float → double` 是浮点提升，`float → int` 是转换。提升的等级高于转换。

### 1.2 复现歧义

另写一组：

```cpp
void rank(long);
void rank(double);
```

调用 `rank(1)`。`int → long` 和 `int → double` 都是 conversion rank，没有谁更好，编译器应该报歧义。

把失败代码放在 `code/fail_overload_ambiguous.cpp`，**不要加入 CMake**。记录完整报错，例如：

```text
error: call of overloaded 'rank(int)' is ambiguous
note: candidate: 'void rank(long int)'
note: candidate: 'void rank(double)'
```

### 1.3 `const` 引用重载

在 `overload.cpp` 里加：

```cpp
void touch(int&);
void touch(const int&);
```

分别用普通左值、`const` 左值、字面量调用：

```cpp
int x = 1;
const int cx = 2;
touch(x);
touch(cx);
touch(3);
```

回答：为什么字面量只能匹配 `const int&`？`int&` 和 `const int&` 的绑定规则差在哪？

### 1.4 `nullptr` 重载

加入 `<cstddef>`，写：

```cpp
void ptr(int*);
void ptr(std::nullptr_t);
```

调用 `ptr(nullptr)`。解释为什么选 `std::nullptr_t` 版本，而不是 `int*` 版本。

### 1.5 默认参数

在 `code/default_args.cpp` 里写：

```cpp
void log(const char* msg, int level = 0, bool flush = false);
```

定义时**不要重复默认值**。分别调用：

```cpp
log("hello");
log("hello", 1);
log("hello", 1, true);
```

然后做两个失败实验：

- `code/fail_default_order.cpp`：`void bad(int a = 1, int b);` —— 默认参数必须从右往左；
- `code/fail_default_redef.cpp`：声明和定义都写默认值 —— 同一个参数不能重复指定默认值。

最后复现“默认参数 + 重载”的歧义：

```cpp
void h(int);
void h(int, int = 0);
h(1);   // 两个候选都能调用
```

把它放进 `fail_overload_ambiguous.cpp` 或单独的失败文件，记录报错。

### 1.6 小结

在笔记里写清：

- 重载解析的三档优先级，以及为什么“提升”能压过“转换”；
- 默认参数属于声明，不属于函数类型；
- 默认参数从右往左，调用点补全；
- 默认参数会让本来不冲突的重载变得歧义。

---

## 任务 2：命名空间、`static`、匿名 namespace（约 50 min）

### 2.1 命名空间基础

在 `code/namespaces.cpp` 里写：

```cpp
namespace math {
int add(int a, int b) { return a + b; }

namespace detail {
int twice(int x) { return x * 2; }
}
}
```

要求：

- 用 `math::add`、`math::detail::twice` 调用；
- 在函数内部用一次 `using math::add;`，再用一次 `using namespace math;`；
- 在笔记里说明 `using` 声明和 `using namespace` 指令的区别，以及为什么头文件里不要写 `using namespace ...;`。

### 2.2 `static` 与匿名 namespace

在同一个文件里加：

```cpp
static int file_local = 7;

namespace {
int hidden_counter = 0;
void bump() { ++hidden_counter; }
}
```

编译成目标文件后看符号：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -c namespaces.cpp -o /tmp/namespaces.o
nm -C /tmp/namespaces.o
```

记录并分类：

- 哪些符号是大写（外部链接）？
- 哪些是小写（局部链接）？
- `math::add`、`math::detail::twice` 出现成什么名字？
- 匿名 namespace 里的 `hidden_counter`、`bump` 在 `nm` 里长什么样？

### 2.3 两个 TU 里的同名内部符号

写 `code/link_internal_a.cpp` 和 `code/link_internal_b.cpp`，两个文件各自定义：

```cpp
static int counter = 0;
namespace {
int helper() { return 42; }
}
```

再各自导出一个外部函数：

```cpp
int a_value();   // a 文件里返回 counter + helper()
int b_value();   // b 文件里返回 counter + helper()
```

分别编译成 `.o` 再链接，确认**不冲突**。用 `nm -C` 看两个 `counter`、`helper` 的符号绑定，并解释：为什么它们不会发生 multiple definition？

### 2.4 `const` 的链接属性

在笔记里解释并验证：

```cpp
const int kShared = 7;          // namespace 作用域，默认内部链接
extern const int kExternal = 7; // 外部链接；这是定义，不能放多个 TU 都包含的头文件
inline constexpr int kInline = 7; // C++17，头文件里的推荐写法
```

如果两个 `.cpp` 都 `#include` 一个定义了 `const int kShared = 7;` 的头文件，为什么链接不报错？每个 TU 是否各有一份？`inline` 变量解决了什么？

注意：`extern const int kExternal = 7;` 带初始化器就是**定义**。把它放头文件里被两个 TU 包含，会得到 multiple definition；正确做法是头文件写 `extern const int kExternal;`，在某个 `.cpp` 里定义一次。

### 2.5 `inline` 函数放头文件

写 `code/link_inline.h`：

```cpp
#pragma once
inline int twice(int x) { return x * 2; }
```

让两个 `.cpp` 都包含它并调用。确认链接成功；再把 `inline` 去掉，观察 multiple definition。这个对比留到任务 3 一起解释。

---

## 任务 3：多文件工程与编译/链接错误（约 60 min）

### 3.1 正常的多文件工程

在 `code/` 下建：

```text
link_stats.h      // 只放声明
link_stats.cpp    // 放定义
link_main.cpp     // main，调用 sum / average
```

`link_stats.h` 至少包含：

```cpp
#pragma once
int sum(const int* data, int n);
double average(const int* data, int n);
```

要求：

- `link_stats.h` 只有声明，不写函数体；
- `link_stats.cpp` 包含头文件并实现；
- `link_main.cpp` 包含头文件，用固定数组测试 `sum`、`average`；
- 所有 `.cpp` 都能单独编译成 `.o`，再链接成可执行文件。

手动构建并观察：

```bash
cd base/days/d03-functions-gdb/code

g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -c link_stats.cpp -o /tmp/link_stats.o
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -c link_main.cpp  -o /tmp/link_main.o

nm -C /tmp/link_main.o | grep -E 'sum|average'
g++ /tmp/link_main.o /tmp/link_stats.o -o /tmp/d03_link
/tmp/d03_link
```

记录 `nm` 输出：`sum`、`average` 在 `link_main.o` 里应该是 `U`（undefined），链接后应该被解析。再对最终可执行文件跑一次 `nm -C /tmp/d03_link | grep ...` 对比。

### 3.2 复现 undefined reference

写 `code/fail_undefined.cpp`：只声明 `int missing_function();`，在 `main` 里调用，不提供定义。

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -c fail_undefined.cpp -o /tmp/fail_undefined.o
g++ /tmp/fail_undefined.o -o /tmp/fail_undefined
```

第一步（编译）应该成功，第二步（链接）报 `undefined reference`。记录完整报错，并解释：为什么编译器当时不知道缺少定义？

### 3.3 复现 multiple definition

写：

- `code/fail_dup.h`：直接定义一个普通函数，例如 `int twice(int x) { return x * 2; }`；
- `code/fail_dup_a.cpp`：包含 `fail_dup.h`，写 `main`；
- `code/fail_dup_b.cpp`：包含 `fail_dup.h`，写一个别的函数。

分别编译成 `.o`（都应成功），再链接，观察 `multiple definition of 'twice(int)'`。记录报错。

然后分别用两种方式修复：

1. 头文件只留声明，定义放到一个 `.cpp`；
2. 函数写成 `inline`，保留在头文件。

回答：

- `#pragma once` / include guard 解决的是同一个 TU 内重复包含，为什么解决不了跨 TU 的 ODR 冲突？
- 为什么类内定义的成员函数默认是 `inline`，而普通函数不是？

### 3.4 编译错误 vs 链接错误

在笔记里做一张表：

| 现象 | 发生在哪个阶段 | 典型原因 | 例子 |
|---|---|---|---|
| `error: expected ';'` |  |  |  |
| 类型不匹配 / 窄化 |  |  |  |
| `undefined reference` |  |  |  |
| `multiple definition` |  |  |  |

结论：编译错误是“这个 TU 自己就有问题”；链接错误是“每个 TU 都编译过了，但符号拼不起来”。

---

## 任务 4：GDB 入门（约 75 min）

### 4.1 准备一个有 bug 的程序

写 `code/debug_me.cpp`，要求：

- 固定数组 `int data[] = {4, 8, 15, 16, 23, 42};`
- `int sum(const int* data, int n)` 求和；
- `double average(const int* data, int n)` 调 `sum` 后除以 `n`；
- **故意留一个逻辑 bug**：例如循环写成 `i < n - 1`，漏掉最后一个元素。不要写越界、空指针这类 UB；今天用“结果不对”来练 GDB。

编译时带调试信息、关闭优化：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -g -O0 \
    debug_me.cpp -o /tmp/d03_debug
```

先运行，记录错误结果（例如 `sum = 66`，而正确值是 `108`）。

### 4.2 必做 GDB 操作

用 `gdb /tmp/d03_debug` 进入，至少完成并记录以下操作：

```text
break main
run
next
print n
x/6dw data

break sum
continue
info args
info locals
next
watch total
continue
bt
frame 1
info locals
frame 0
finish
quit
```

要求：

- 用 `bt` 看到至少两层栈帧：`sum` 和 `main`；
- 用 `frame 1` 切到调用者，`info locals` 看 `data`、`n`；
- 用 `watch total` 观察变量什么时候被修改；
- 用 `display i` 或 `display total` 持续显示变量；
- 用 `finish` 从 `sum` 返回到 `main`，看返回值；
- 找到 bug 后修改源码，重新编译运行，确认 `sum = 108`、`average = 18`。

### 4.3 额外练习

- 条件断点：`break sum if n == 6`；
- 查看数组：`print *data@6` 或 `x/6dw data`；
- 运行中改变量：`set var i = 5`（观察对循环的影响，仅实验）；
- 用 `-O2 -g` 再编译一次，尝试 `print` 局部变量，观察 `<optimized out>`。解释为什么优化后调试信息会变差。

### 4.4 GDB 记录

在 `note.md` 里放一段真实会话（可以截取），至少包含：

- 断点位置；
- `bt` 的完整输出；
- `watch` 命中时的旧值/新值；
- 你如何定位到 `i < n - 1`；
- 修复前后的运行结果。

---

## 任务 5：Git 分支与合并（约 35 min）

### 5.1 在分支上完成 D03

```bash
git switch -c d03-functions-gdb
git status
git add base/days/d03-functions-gdb base/CMakeLists.txt
git commit -m "d03: 函数、链接与 GDB"
```

### 5.2 合并回 main

```bash
git switch main
git merge --no-ff d03-functions-gdb -m "merge: d03 函数、链接与 GDB"
git log --oneline --graph --all
```

要求：

- 使用 `--no-ff`，保留“这里合并过一个分支”的历史；
- 观察 fast-forward 和 `--no-ff` 生成的历史差异；
- 合并完成后删除分支：`git branch -d d03-functions-gdb`。

### 5.3 冲突练习（推荐）

在一个临时文件上制造一次冲突，练习解决：

```bash
git switch -c d03-conflict
printf 'branch\n' > merge_practice.txt
git add merge_practice.txt
git commit -m "d03: conflict branch"

git switch main
printf 'main\n' > merge_practice.txt
git add merge_practice.txt
git commit -m "d03: conflict main"

git merge d03-conflict
```

合并会停在冲突状态。手工编辑 `merge_practice.txt` 成最终内容，然后：

```bash
git add merge_practice.txt
git commit -m "d03: resolve conflict"
git switch main
git branch -d d03-conflict
```

如果不想保留这个练习文件，解决冲突后 `git rm merge_practice.txt` 再提交。记录 `git status` 在冲突时的输出（`both modified`）。

---

## 任务 6：CMake、笔记与收尾（约 30 min）

在 `base/CMakeLists.txt` 中为每个带 `main` 的练习文件加独立 target：

| target | 源文件 |
|---|---|
| `d03-overload` | `overload.cpp` |
| `d03-defaults` | `default_args.cpp` |
| `d03-namespaces` | `namespaces.cpp` |
| `d03-linkage` | `link_main.cpp` + `link_stats.cpp` |
| `d03-debug` | `debug_me.cpp`，额外 `-g -O0` |

要求：

- 全部 C++20 + `-Wall -Wextra -Wpedantic -Wconversion`；
- `fail_*.cpp` **不要**加进 CMake，它们本来就编译/链接失败；
- `d03-debug` 加 `-g -O0`，方便 GDB；
- 零警告构建。

构建与运行：

```bash
cmake -S /mnt/d/Project/CPP/CPPLearn/base -B ~/build/base
cmake --build ~/build/base --target d03-overload d03-defaults d03-namespaces d03-linkage d03-debug -j"$(nproc)"
~/build/base/d03-overload
~/build/base/d03-linkage
```

`note.md` 按模板写，至少包含：

- 重载解析优先级表和 `const` / `nullptr` 实验结果；
- 默认参数规则、失败报错；
- 命名空间与链接属性；`nm` 符号分类；
- 编译错误 vs 链接错误的表和两个最小复现；
- GDB 会话、`bt` / `frame` / `watch` 的实际输出、bug 修复前后结果；
- Git 分支图和冲突解决记录；
- 3–5 条易错点。

---

## 产出清单

- `code/overload.cpp`、`default_args.cpp`、`namespaces.cpp`；
- `code/link_stats.h`、`link_stats.cpp`、`link_main.cpp`；
- `code/debug_me.cpp` 及修复后的版本；
- `code/fail_overload_ambiguous.cpp`、`fail_default_order.cpp`、`fail_default_redef.cpp`、`fail_undefined.cpp`、`fail_dup*.cpp`（失败实验，不进 CMake）；
- `note.md`；
- `base/CMakeLists.txt` 中 5 个新 target；
- 至少一次提交，且 D03 分支已合并回 `main`。

## 验收标准

- [ ] 能解释 `pick('a')`、`pick(1.0f)` 为什么选中对应重载；
- [ ] 能复现并解释重载歧义、默认参数顺序错误、默认参数重复定义；
- [ ] 能说清 `static`、匿名 namespace、`const` 的链接属性；
- [ ] 多文件工程能手动分步编译、链接，`nm` 能看到 `U` 被解析；
- [ ] 能复现 `undefined reference` 和 `multiple definition` 并修复；
- [ ] GDB 中能独立使用 `break`、`next`、`step`、`bt`、`frame`、`watch`、`finish`；
- [ ] 修复后的 `debug_me` 输出正确；
- [ ] Git 分支已合并，`git log --graph` 清晰；
- [ ] 严格警告下零警告。

## 自测问题

1. 重载解析的精确匹配、提升、转换三档，能各举一个例子吗？
2. 为什么 `pick('a')` 选 `pick(int)`，而不是 `pick(double)`？
3. 默认参数为什么必须从右往左？声明和定义重复写默认值会怎样？
4. `void h(int); void h(int, int = 0); h(1);` 为什么有歧义？
5. namespace 作用域的 `static` 和匿名 namespace 有什么共同点？区别是什么？
6. 为什么 `const int k = 7;` 放在头文件里，两个 TU 包含后链接不报重复定义？
7. `#pragma once` 能防止什么？不能防止什么？
8. `nm` 输出里 `T`、`U`、`t` 分别代表什么？
9. 编译错误和链接错误分别在什么时候发生？各举两个原因。
10. GDB 的 `bt`、`frame`、`watch`、`finish` 各自解决什么问题？
11. 为什么 GDB 调试要用 `-g -O0`？`-O2` 后局部变量为什么可能显示 `<optimized out>`？
12. `git merge --no-ff` 和默认 fast-forward 的区别是什么？

## 完成后

告诉我「D03 完成」，并附上：

- `cmake --build` 的完整输出（确认零警告）；
- `note.md`，特别是 GDB 会话和链接错误记录；
- `git log --oneline --graph --all`；
- 修复前后的 `debug_me` 输出。

我会检查你是否真的用 GDB 走过调用栈，而不是只看代码猜 bug；确认后再进入 D04（类与对象 I：生命周期）。
