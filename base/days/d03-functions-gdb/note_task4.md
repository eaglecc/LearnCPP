# D03 任务 4：GDB 入门

> 环境：WSL2 Ubuntu 24.04，g++ 13.3.0，GDB 15.1。
> 代码：[code/debug_me.cpp](code/debug_me.cpp)
>
> 本笔记记录的是实际编译、运行和 GDB 会话的输出。当前代码的数组是 5 个元素
> `{1, 2, 3, 4, 5}`，不是任务书示例的 6 个元素；预期值会跟着变，下面会分别说明。

## 0. 日常 GDB 命令速查

### 0.1 编译并启动

```bash
# 带调试信息、关闭优化
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -g -O0 debug_me.cpp -o /tmp/d03_debug

# 启动 GDB
gdb -q /tmp/d03_debug
```

### 0.2 最常用命令表

| 分类 | 命令 | 作用 |
|---|---|---|
| 启动 | `gdb -q ./prog` | 加载程序，`-q` 不显示欢迎信息 |
| 启动 | `gdb --args ./prog arg1 arg2` | 带命令行参数启动 |
| 运行 | `run` / `r` | 开始运行；也可写 `r arg1 arg2` |
| 运行 | `continue` / `c` | 继续到下一个断点或观察点 |
| 退出 | `quit` / `q` | 退出 GDB |
| 断点 | `break main` / `b main` | 在 `main` 入口下断点 |
| 断点 | `break sum` | 在函数 `sum` 下断点 |
| 断点 | `break debug_me.cpp:7` | 在指定源码行下断点 |
| 断点 | `break sum if n == 5` | 条件断点 |
| 断点 | `info breakpoints` / `i b` | 查看所有断点及编号 |
| 断点 | `disable 2` / `enable 2` | 禁用/启用编号为 2 的断点 |
| 断点 | `delete 2` / `d 2` | 删除编号为 2 的断点 |
| 单步 | `next` / `n` | 执行下一行，不进入函数 |
| 单步 | `step` / `s` | 执行下一行，进入函数 |
| 单步 | `finish` | 执行到当前函数返回，并打印返回值 |
| 查看 | `print n` / `p n` | 打印变量或表达式 |
| 查看 | `print *data@5` | 打印从 `data` 开始的 5 个元素 |
| 查看 | `x/5dw data` | 按内存查看 5 个十进制 word |
| 查看 | `info args` | 查看当前函数参数 |
| 查看 | `info locals` | 查看当前函数局部变量 |
| 查看 | `list` / `l` | 查看当前源码附近的代码 |
| 观察 | `watch total` | 变量被写时停下，显示 old/new |
| 观察 | `display i` | 每次停下自动打印 `i` |
| 观察 | `undisplay 1` | 取消编号为 1 的自动显示 |
| 调用栈 | `bt` | 查看调用栈 |
| 调用栈 | `frame 1` / `f 1` | 切换到第 1 层栈帧 |
| 调用栈 | `up` / `down` | 在调用栈中上移/下移一层 |
| 改变量 | `set var i = 3` | 运行时改变量，仅用于实验 |
| 配置 | `set debuginfod enabled off` | 关闭 debuginfod 提示，输出更干净 |
| 帮助 | `help break` | 查看某条命令的帮助 |

### 0.3 日常调试主线

大多数时候只需要这一条链：

```text
gdb -q /tmp/d03_debug
b main          # 先停在入口
r
n               # 单步，不进入函数
p n             # 看变量
b sum           # 对可疑函数下断点
c               # 继续到 sum
info args       # 看参数
info locals     # 看局部变量
bt              # 看谁调用了 sum
f 1             # 切到调用者 main
p data          # 看 main 里的数组
f 0             # 回到 sum
finish          # 看 sum 返回值
q
```

记住三个单步命令的分工：

| 命令 | 记忆方法 |
|---|---|
| `next` | 下一步，**不进去** |
| `step` | 下一步，**进去** |
| `finish` | 跑完当前函数，**出来** |

> **记忆点：** 日常调试主线是 `b → r → n/s → p → bt → c`；先停下来再看变量，不要从 `main` 一路单步到底。

## 1. 代码与 bug

### 1.1 当前代码

```cpp
#include <iostream>

namespace {
    int sum(const int *data, int n) {
        int total = 0;
        for (int i = 0; i < n - 1; ++i) {
            total += data[i];
        }
        return total;
    }

    double average(const int *data, int n) {
        if (n <= 0) {
            return 0.0;
        }
        return static_cast<double>(sum(data, n)) / static_cast<double>(n);
    }
}

int main() {
    const int data[] = {1, 2, 3, 4, 5};
    int n = sizeof(data) / sizeof(data[0]);
    std::cout << "Sum: " << sum(data, n) << std::endl;
    std::cout << "Average: " << average(data, n) << std::endl;
    return 0;
}
```

关键行是第 6 行：

```cpp
for (int i = 0; i < n - 1; ++i)
```

`i < n - 1` 让循环少执行一次：

- `n = 5` 时，`i` 只取 `0, 1, 2, 3`；
- `data[4]` 也就是最后一个元素 `5` 没有被加进去；
- 所以 `sum` 得到 `1 + 2 + 3 + 4 = 10`，而不是 `15`。

### 1.2 实际运行结果

编译并运行：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -g -O0 debug_me.cpp -o /tmp/d03_debug
/tmp/d03_debug
```

实际输出：

```text
Sum: 10
Average: 2
```

正确结果应该是：

| 数组 | 错误 sum | 正确 sum | 错误 average | 正确 average |
|---|---|---|---|---|
| 当前 `{1,2,3,4,5}` | 10 | 15 | 2 | 3 |
| 任务书 `{4,8,15,16,23,42}` | 66 | 108 | 11 | 18 |

`average` 内部调用 `sum`，所以 `sum` 的错误会传递到 `average`。

> **记忆点：** 先写清楚输入和预期输出，再看实际结果；调试不是“凭感觉改代码”。

### 1.3 修复

把循环上界从 `n - 1` 改成 `n`：

```cpp
for (int i = 0; i < n; ++i) {
    total += data[i];
}
```

为了验证修复，用临时副本编译运行（仓库里的 `debug_me.cpp` 仍保留 bug 供练习）：

```bash
sed 's/i < n - 1/i < n/' debug_me.cpp > /tmp/debug_me_fixed.cpp
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -g -O0 /tmp/debug_me_fixed.cpp -o /tmp/d03_debug_fixed
/tmp/d03_debug_fixed
```

实际输出：

```text
Sum: 15
Average: 3
```

如果换成任务书的 6 元素数组，修复后就是 `Sum: 108`、`Average: 18`。

> **记忆点：** 循环漏掉最后一个元素，优先检查上界是 `< n` 还是 `< n - 1`。

## 2. 编译带调试信息的版本

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -g -O0 debug_me.cpp -o /tmp/d03_debug
```

- `-g`：生成调试信息，让 GDB 能把机器码对应回源码行号和变量名；
- `-O0`：关闭优化，变量通常放在栈上，便于 `print`；
- 严格警告仍然保留，正常代码零警告。

启动 GDB：

```bash
gdb -q /tmp/d03_debug
```

> **记忆点：** `-g` 决定“有没有调试信息”，`-O0` 决定“变量还在不在”；学习调试先用 `-g -O0`。

## 3. GDB 会话

### 3.1 在 `main` 停下来看数组

命令：

```text
break main
run
next
next
next
print n
x/5dw data
```

实际会话（省略了 debuginfod 提示）：

```text
Breakpoint 1, main () at debug_me.cpp:21
21	int main() {
22	    const int data[] = {1, 2, 3, 4, 5};
23	    int n = sizeof(data) / sizeof(data[0]);
24	    std::cout << "Sum: " << sum(data, n) << std::endl;
$1 = 5
0x7fffffffe3b0:	1	2	3	4
0x7fffffffe3c0:	5
```

解释：

- `break main` 在 `main` 入口停下；
- `run` 启动程序；
- 连续 `next` 执行完 `data` 和 `n` 的初始化；
- `print n` 得到 `5`；
- `x/5dw data` 从 `data` 地址开始，按十进制、4 字节 word 查看 5 个元素；
- 输出分成两行只是 GDB 的排版，不是两个数组。

`x/5dw` 的格式：

| 部分 | 含义 |
|---|---|
| `x` | examine memory，查看内存 |
| `5` | 查看 5 个元素 |
| `d` | decimal，有符号十进制 |
| `w` | word，4 字节 |

等价写法：

```text
print *data@5
```

实际输出：

```text
$1 = {1, 2, 3, 4, 5}
```

`@` 表示“从 `data` 开始连续取 5 个元素”。

> **记忆点：** 数组有几个元素就查几个；当前是 5 个，不要照抄任务书的 `x/6dw`，否则会读到数组末尾之外。

### 3.2 进入 `sum`，看参数和局部变量

命令：

```text
break sum
continue
info args
next
info locals
next
display i
print total
next
print total
```

实际会话：

```text
Breakpoint 2, (anonymous namespace)::sum (data=0x7fffffffe3b0, n=5) at debug_me.cpp:5
5	        int total = 0;
data = 0x7fffffffe3b0
n = 5
6	        for (int i = 0; i < n - 1; ++i) {
i = 0
total = 0
7	            total += data[i];
$1 = 0
6	        for (int i = 0; i < n - 1; ++i) {
1: i = 0
$2 = 1
```

解释：

- `sum` 在匿名 namespace 里，所以 GDB 显示 `(anonymous namespace)::sum`；
- `info args` 显示参数 `data` 和 `n`；
- `info locals` 显示局部变量；此时 `i = 0`、`total = 0`；
- `next` 从第 7 行执行到第 6 行，`total += data[0]` 执行后 `total` 变成 1；
- `display i` 让 GDB 每次停下来自动打印 `i`，这里显示 `1: i = 0`。

注意 `i` 是 `for` 初始化里声明的：

```cpp
for (int i = 0; ...)
```

在第 5 行还没进入 `for` 时，`i` 还不在作用域里；在 `for` 初始化之后才能 `print i`。如果在第 5 行直接 `display i`，会看到：

```text
No symbol "i" in current context.
```

> **记忆点：** `info args` 看参数，`info locals` 看局部变量；变量还没进入作用域时，GDB 会说 no symbol。

### 3.3 `watch` 观察 `total` 被写

命令：

```text
break sum
run
next
watch total
continue
```

实际会话：

```text
Breakpoint 1, (anonymous namespace)::sum (data=0x7fffffffe3b0, n=5) at debug_me.cpp:5
5	        int total = 0;
6	        for (int i = 0; i < n - 1; ++i) {
Hardware watchpoint 2: total

Hardware watchpoint 2: total

Old value = 0
New value = 1
(anonymous namespace)::sum (data=0x7fffffffe3b0, n=5) at debug_me.cpp:6
6	        for (int i = 0; i < n - 1; ++i) {
```

解释：

- `watch total` 不是“打印 total”，而是“当 total 被写时停下来”；
- 第一次写入是 `total += data[0]`，所以 old=0、new=1；
- GDB 显示 `Hardware watchpoint`，说明用了硬件观察点；
- 观察点适合找“这个变量到底在哪一行被改了”。

> **记忆点：** `watch` 抓的是写操作，命中时看 old/new 就知道是哪一行改的。

### 3.4 `bt` 看调用栈，`frame` 切栈帧

命令：

```text
bt
frame 1
info locals
frame 0
```

实际会话：

```text
#0  (anonymous namespace)::sum (data=0x7fffffffe3b0, n=5) at debug_me.cpp:6
#1  0x00005555555552cf in main () at debug_me.cpp:24
#1  0x00005555555552cf in main () at debug_me.cpp:24
24	    std::cout << "Sum: " << sum(data, n) << std::endl;
data = {1, 2, 3, 4, 5}
n = 5
#0  (anonymous namespace)::sum (data=0x7fffffffe3b0, n=5) at debug_me.cpp:6
6	        for (int i = 0; i < n - 1; ++i) {
```

解释：

- `#0` 是当前函数 `sum`；
- `#1` 是调用者 `main`；
- `frame 1` 切到 `main`，再 `info locals` 就能看到 `main` 的 `data` 和 `n`；
- `frame 0` 切回 `sum`。

地址 `0x000055...` 每次运行可能不同，不用记；看函数名和行号即可。

> **记忆点：** `bt` 是“我在哪、谁调用我”；`frame N` 在调用链上上下移动。

### 3.5 `finish` 看返回值

命令：

```text
break sum
run
finish
```

实际会话：

```text
Breakpoint 1, (anonymous namespace)::sum (data=0x7fffffffe3b0, n=5) at debug_me.cpp:5
5	        int total = 0;
0x00005555555552cf in main () at debug_me.cpp:24
24	    std::cout << "Sum: " << sum(data, n) << std::endl;
Value returned is $1 = 10
```

`finish` 会一直执行到当前函数返回，然后：

- 回到调用者 `main` 的第 24 行；
- 打印 `sum` 的返回值 `10`；
- 这个值正好暴露了 bug：正确值应是 15。

注意：`finish` 不是“无条件跑完”。如果过程中碰到断点或 watchpoint，会先停下来。实际会话里如果 `watch total` 还开着，`finish` 可能先命中 watchpoint；可以先 `disable <编号>`，再 `finish`。

> **记忆点：** `finish` 只跑完当前函数，并告诉你返回值；遇到断点/观察点仍会先停。

### 3.6 观察循环变量 `i`

命令：

```text
break debug_me.cpp:7
run
print i
continue
print i
continue
print i
continue
print i
continue
```

实际会话（第一次 `sum` 调用）：

```text
Breakpoint 1, (anonymous namespace)::sum (data=0x7fffffffe3b0, n=5) at debug_me.cpp:7
7	            total += data[i];
$1 = 0

Breakpoint 1, (anonymous namespace)::sum (data=0x7fffffffe3b0, n=5) at debug_me.cpp:7
7	            total += data[i];
$2 = 1

Breakpoint 1, (anonymous namespace)::sum (data=0x7fffffffe3b0, n=5) at debug_me.cpp:7
7	            total += data[i];
$3 = 2

Breakpoint 1, (anonymous namespace)::sum (data=0x7fffffffe3b0, n=5) at debug_me.cpp:7
7	            total += data[i];
$4 = 3
Sum: 10
```

`i` 只到 `3`，没有 `4`，直接证明循环漏掉了 `data[4]`。

后面还有一次 `sum` 断点命中，是因为 `average` 内部又调用了一次 `sum`：

```cpp
return static_cast<double>(sum(data, n)) / static_cast<double>(n);
```

> **记忆点：** 循环变量的实际取值最能暴露边界错误；`i` 少一次就是漏了最后一个元素。

### 3.7 在 `return` 处看总和和最后一个元素

命令：

```text
break debug_me.cpp:9
run
print total
print data[4]
print n
```

实际输出：

```text
$1 = 10
$2 = 5
$3 = 5
```

解释：

- `total = 10`，只加了前四个；
- `data[4] = 5`，最后一个元素没有参与；
- `n = 5`，说明数组确实有 5 个元素。

如果循环是 `i < n`，`total` 应为 `10 + 5 = 15`。

> **记忆点：** 在 `return` 处同时看累加结果和最后一个元素，能快速确认“是不是漏了最后一个”。

### 3.8 条件断点

只希望 `n == 5` 时停在 `sum`：

```text
break sum if n == 5
run
info args
```

实际输出：

```text
Breakpoint 1, (anonymous namespace)::sum (data=0x7fffffffe3b0, n=5) at debug_me.cpp:5
5	        int total = 0;
data = 0x7fffffffe3b0
n = 5
```

如果条件写成 `n == 6`，这个程序不会命中，因为当前数组是 5 个元素。

> **记忆点：** 条件断点把“停不停”交给表达式，适合在多次调用中只抓特定参数的那一次。

### 3.9 运行中改变量：`set var`

在循环体里把 `i` 改成 3：

```text
break sum
run
next
next
print i
set var i = 3
print i
next
print total
next
print total
finish
```

实际会话：

```text
Breakpoint 1, (anonymous namespace)::sum (...) at debug_me.cpp:5
5	        int total = 0;
6	        for (int i = 0; i < n - 1; ++i) {
7	            total += data[i];
$1 = 0
$2 = 3
6	        for (int i = 0; i < n - 1; ++i) {
$3 = 4
9	        return total;
$4 = 4
Value returned is $5 = 4
```

解释：

- 原本 `i = 0`；
- `set var i = 3` 把循环变量改成 3；
- 下一次循环体加的是 `data[3] = 4`；
- 所以 `total = 4`，`finish` 返回值也是 4。

这是调试实验，不是修复代码；它说明 GDB 可以临时改变量来验证假设。

> **记忆点：** `set var` 改的是运行中的值，不会改源码；适合做“如果这里是 X 会怎样”的实验。

### 3.10 `-O2 -g` 后变量被优化掉

编译：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -g -O2 debug_me.cpp -o /tmp/d03_debug_O2
```

尝试在 `sum` 下断点：

```text
break sum
run
```

实际输出：

```text
Function "sum" not defined.
Make breakpoint pending on future shared library load? (y or [n]) [answered N; input not from terminal]
```

因为 `sum` 被内联了，已经没有独立的 `sum` 函数帧。

改在 `main` 看变量：

```text
break main
run
info locals
print n
print data[0]
```

实际输出（`info locals` 与 `print` 的观察）：

```text
Breakpoint 1, main () at debug_me.cpp:21
21	int main() {
data = <optimized out>
n = 5
$1 = 5
$2 = <optimized out>
```

其中 `data` 已经无法从调试信息里还原，`n` 仍然能读到 5，`print data[0]` 得到 `<optimized out>`。

解释：

- `-g` 只负责生成调试信息；
- `-O2` 会把函数内联、把变量放进寄存器，甚至直接消除；
- GDB 仍然知道源码行号，但有些变量已经无法还原；
- 所以学习调试用 `-O0`，发布优化版本再用 `-O2`。

> **记忆点：** `-g` 不等于“变量一定可见”；优化会内联函数、消除变量，调试先用 `-O0`。

## 4. GDB 命令速查

| 命令 | 作用 |
|---|---|
| `gdb -q /tmp/d03_debug` | 启动 GDB，加载可执行文件 |
| `break main` / `b main` | 在 `main` 入口下断点 |
| `break sum` | 在函数 `sum` 下断点 |
| `break debug_me.cpp:7` | 在指定源码行下断点 |
| `break sum if n == 5` | 条件断点 |
| `run` / `r` | 启动程序 |
| `next` / `n` | 单步，不进入函数 |
| `step` / `s` | 单步，进入函数 |
| `continue` / `c` | 继续到下一个断点/观察点 |
| `print n` / `p n` | 打印变量 |
| `print *data@5` | 打印从 `data` 开始的 5 个元素 |
| `x/5dw data` | 按内存查看 5 个十进制 word |
| `info args` | 查看当前函数参数 |
| `info locals` | 查看当前函数局部变量 |
| `watch total` | 观察变量被写 |
| `display i` | 每次停下自动打印 `i` |
| `undisplay 1` | 取消编号为 1 的 display |
| `bt` | 查看调用栈 |
| `frame 1` | 切换到第 1 层栈帧（调用者） |
| `frame 0` | 切回当前函数 |
| `finish` | 执行到当前函数返回 |
| `quit` / `q` | 退出 GDB |

> **记忆点：** `next` 跨过函数，`step` 进入函数；`continue` 到下一个断点，`finish` 跑完当前函数。

## 5. 怎么从现象定位到 bug

1. 运行程序，得到 `Sum: 10`、`Average: 2`；
2. 根据输入 `{1,2,3,4,5}` 手算正确值：`15`、`3`；
3. 在 `main` 确认 `n = 5`、数组内容正确；
4. 在 `sum` 确认参数 `data`、`n` 正确；
5. 在 `return total` 处看到 `total = 10`，而 `data[4] = 5` 没加；
6. 在循环体断点观察 `i` 只到 `3`，没有 `4`；
7. 回到源码，发现循环条件是 `i < n - 1`；
8. 改成 `i < n`，重新编译运行，得到 `15`、`3`。

关键不是“GDB 帮你自动找 bug”，而是：

- 先用预期值判断结果不对；
- 再用断点/watch 把“错误结果”缩小到某一行；
- 最后回到源码确认逻辑。

> **记忆点：** 调试流程是“预期 → 实际 → 缩小范围 → 看变量 → 回到源码”，不是盲目单步。

## 6. 易错点

1. **只编译不加 `-g`**：GDB 看不到源码行号和变量，调试体验很差。
2. **用 `-O2` 学调试**：函数被内联、变量被优化掉，`print` 经常得到 `<optimized out>`。
3. **在变量初始化前 `print`**：例如 `break main` 后立刻 `print n`，可能得到 0 或未初始化值；先 `next` 到初始化之后。
4. **把 `x/5dw` 当成“5 个字节”**：`w` 是 4 字节 word，`5` 是元素个数。
5. **数组只有 5 个元素却写 `x/6dw`**：会读到数组末尾之外；调试读取不等于程序越界，但结果没有意义。
6. **`display i` 在 `i` 进入作用域前设置**：会提示 `No symbol "i" in current context`；先进入 `for`。
7. **`finish` 时还开着 watchpoint**：可能先命中观察点而不是返回；先 `disable` 观察点。
8. **把 `watch` 当成 `print`**：`watch` 只在变量被写时停，不会持续打印。
9. **`next` 和 `step` 混用**：`next` 不进入函数，`step` 进入；想看 `sum` 内部要用 `step` 或直接 `break sum`。
10. **只看 GDB 不看源码**：GDB 告诉你“值是多少”，最终还要回到源码确认“为什么”。

> **记忆点：** GDB 是放大镜，不是修 bug 的自动工具；变量可见性和优化级别先配对，再谈单步。

## 7. 完整复现

```bash
cd base/days/d03-functions-gdb/code

# 编译调试版
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror \
    -g -O0 debug_me.cpp -o /tmp/d03_debug
/tmp/d03_debug
# 预期错误输出：Sum: 10 / Average: 2

# 启动 GDB
gdb -q /tmp/d03_debug
```

在 GDB 中至少执行：

```text
break main
run
next
next
next
print n
x/5dw data

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

然后修复源码：

```cpp
- for (int i = 0; i < n - 1; ++i) {
+ for (int i = 0; i < n; ++i) {
```

重新编译运行，当前 5 元素数组应得到：

```text
Sum: 15
Average: 3
```

如果使用任务书的 6 元素数组 `{4,8,15,16,23,42}`，则应得到 `Sum: 108`、`Average: 18`。

## 8. 验收

- [x] 用 `-g -O0` 编译并运行，记录错误结果 `Sum: 10`、`Average: 2`；
- [x] `break main` 后能看到 `n = 5` 和数组内容；
- [x] `break sum` 后 `info args`、`info locals` 正常；
- [x] `watch total` 命中并显示 old/new；
- [x] `bt` 至少看到 `sum` 和 `main` 两层；
- [x] `frame 1` 能查看 `main` 的 `data`、`n`；
- [x] `finish` 显示 `sum` 返回值为 10；
- [x] 条件断点 `break sum if n == 5` 命中；
- [x] `-O2 -g` 观察到 `sum` 被内联、`data` 为 `<optimized out>`；
- [x] 修复后当前数组输出 `Sum: 15`、`Average: 3`；
- [x] 把 `debug_me.cpp` 加入 CMake 的 `d03-debug` target（任务 6 已完成）。

## 9. 待复习

- `-g` 和 `-O0` 分别做什么？为什么调试时两个都要？
- `next`、`step`、`continue`、`finish` 的区别是什么？
- `break main`、`break sum`、`break file:line`、条件断点分别怎么用？
- `info args` 和 `info locals` 的区别？
- `watch` 和 `display` 的区别？
- `bt`、`frame 0`、`frame 1` 分别表示什么？
- `x/5dw` 中 `5`、`d`、`w` 各是什么含义？
- 为什么 `-O2` 下 `print data[0]` 会得到 `<optimized out>`？
- 这个 bug 为什么是逻辑错误而不是 UB？
- 如何用 GDB 证明循环少执行了一次？
