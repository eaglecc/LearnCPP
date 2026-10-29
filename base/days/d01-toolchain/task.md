# D01 任务：环境 + 编译模型 + Git

> 预计 4.5–5 小时。按「概念 1h / 动手 2.5h / 笔记 1h」推进。
> 今天的核心不是写多少代码，而是**把工具链跑通，并亲眼看到 `.cpp` 到可执行文件中间发生了什么**。

## 今日目标

- [ ] Ubuntu 虚拟机内工具链齐备且版本达标
- [ ] 能亲手复现「预处理 → 编译 → 汇编 → 链接」四个阶段并解释每步产物
- [ ] 能用最小例子复现并修复 ODR / multiple definition 链接错误
- [ ] 有一个能构建、能运行的 CMake 工程（C++20 + 严格警告）
- [ ] Git 仓库初始化完成，有当天的提交
- [ ] 写完 `docs/toolchain.md` 和本目录 `note.md`

## 今日不碰

类、动态内存、模板、STL 容器、多线程、网络。遇到看不懂的语法先记到「待复习」，不要扩散。

---

## 任务 1：工具链检查（约 30 min）

在 Ubuntu 中逐项检查并记录版本：

```bash
g++ --version
cmake --version
ninja --version
gdb --version
git --version
```

要求：GCC ≥ 13、CMake ≥ 3.28、Ninja、GDB、Git 均可用。缺失时安装：

```bash
sudo apt update
sudo apt install -y build-essential cmake ninja-build gdb git
```

验证 C++20 可用（标准库头 + 新特性）：

```bash
echo '#include <version>
int main() { return 0; }' | g++ -std=c++20 -x c++ - -o /tmp/check && /tmp/check && echo OK
```

**产出**：把版本信息记入 `docs/toolchain.md`。

## 任务 2：共享目录与 SSH 接入（约 30 min，若已配好则跳过并记录）

按 README 第 2 节完成：

1. VirtualBox 共享文件夹挂载 `D:\Project\CPP\CPPLearn` → `/media/sf_CPPLearn`；
2. `sudo usermod -aG vboxsf $USER`，**重新登录**后验证可读写；
3. 建软链接：`ln -s /media/sf_CPPLearn ~/CPPLearn`；
4. Windows 端 VS Code Remote-SSH 连上虚拟机，打开 `~/CPPLearn/base`。

验证：在 VS Code 里新建一个文件、保存，回 Windows 能看到；反之亦然。

**关键约定**：源码在共享目录，**构建目录放虚拟机原生磁盘**（`~/build/...`），不要放 `~/CPPLearn` 下。

## 任务 3：编译四阶段实验（约 60 min）

在本目录写一个 `hello.cpp`（至少包含一个 `#include <iostream>` 和一个自定义函数），然后分四步构建：

```bash
g++ -std=c++20 -E hello.cpp -o hello.i     # 预处理
g++ -std=c++20 -S hello.i   -o hello.s     # 编译
g++ -std=c++20 -c hello.s   -o hello.o     # 汇编
g++ hello.o -o hello                        # 链接
./hello
```

观察并回答（写进 `note.md`）：

1. `.i` 文件比源文件大了多少？`#include` 被展开成了什么？
2. `.s` 是给人看的还是给机器看的？加 `-O0` 和 `-O2` 各生成一次，`main` 的差别是什么？
3. `.o` 里有什么？用 `nm -C hello.o` 看符号，哪些是 `T`、哪些是 `U`？
4. `file hello.o` 和 `file hello` 分别输出什么？
5. `ldd ./hello` 显示了什么？程序运行时依赖谁？
6. `objdump -d hello.o | head -50` 能看懂哪几条指令？

再做一次**多文件**版本：

- `greet.h`：函数声明；
- `greet.cpp`：函数定义（`#include "greet.h"`）；
- `main.cpp`：`#include "greet.h"` 并调用。

分别编译成 `.o`，再链接成一个可执行文件。观察 `nm` 输出中未定义符号（`U`）如何在链接后被解析。

## 任务 4：ODR 实验——复现 multiple definition（约 45 min）

**目的**：分清「编译错误」和「链接错误」，理解头文件里「声明 vs 定义」。

复现步骤：

1. 新建 `odr_shared.h`，在里面**定义**一个普通函数（不是声明），例如 `int add(int a, int b) { return a + b; }`；
2. 新建 `odr_a.cpp` 和 `odr_b.cpp`，都 `#include "odr_shared.h"`，各自写一个函数；其中 `odr_a.cpp` 带 `main`；
3. 两个 `.cpp` 分别编译成 `.o`（**都不报错**），再链接 → 观察 `multiple definition of ...`；
4. 修复方式一：头文件只留声明，定义移到某一个 `.cpp` 中；
5. 修复方式二：把函数写成 `inline`，保留在头文件中；
6. 记录：`#pragma once` 或 include guard **为什么不能**解决这个链接错误？

写进 `note.md`，附上最小复现代码和你看到的完整报错。

> 提示：类内定义的成员函数默认是 `inline` 的，所以头文件里写类往往不出问题——这也是 D4 的伏笔，先记住这个现象。

## 任务 5：首个 CMake 工程（约 60 min）

在 `base/CMakeLists.txt` 建顶层工程，要求：

- `cmake_minimum_required(VERSION 3.28)`；
- `project(... LANGUAGES CXX)`；
- C++20 且 `CMAKE_CXX_STANDARD_REQUIRED ON`、`CMAKE_CXX_EXTENSIONS OFF`；
- 统一警告：`-Wall -Wextra -Wpedantic -Wconversion`（用 `add_compile_options` 或 `target_compile_options`）；
- 一个可执行目标，源码取任务 3 的多文件版本（`main.cpp` + `greet.cpp`）；
- **每个带 `main` 的练习文件必须各自独立成 target**，不要把所有 `.cpp` 塞进同一个可执行文件（否则会 `main` 重定义）。

构建与运行（构建目录在虚拟机原生磁盘）：

```bash
cmake -S ~/CPPLearn/base -B ~/build/base -G Ninja
cmake --build ~/build/base -j
~/build/base/<你的可执行名>
```

要求：**零警告**通过。首次构建后看一眼 `compile_commands.json` 是否需要生成（`CMAKE_EXPORT_COMPILE_COMMANDS`），为 VS Code 补全做准备。

> 今天只要求单目标能构建。`add_library`、Ctest、Presets 留到 D7。

## 任务 6：Git 初始化与提交（约 30 min）

在 `base/` 下：

```bash
git config user.name  "你的名字"
git config user.email "你的邮箱"
git init
git status
git add .
git diff          # 看工作区 vs 暂存区
git diff --staged # 看暂存区 vs HEAD（第一次提交前为空）
git commit -m "d01: 环境 + 编译模型 + Git"
git log --oneline --graph
```

先写 `.gitignore`，至少忽略：

```
build/
*.o
*.i
*.s
compile_commands.json
.cache/
```

要求：

- 至少 2 次提交（例如「CMake 工程跑通」一次，「笔记完成」一次）；
- 提交信息格式 `dNN: 主题`；
- 提交前用 `git status` 确认没有把构建产物、临时文件带进去。

## 任务 7：笔记（约 45–60 min）

完成本目录 `note.md`（模板已给），并撰写 `docs/toolchain.md`，内容至少包括：

1. 工具清单与版本（GCC / CMake / Ninja / GDB / Git）；
2. 编译四阶段：命令 → 产物 → 每阶段做什么；
3. 常用观察命令：`nm -C`、`objdump -d`、`file`、`ldd` 各看什么；
4. 共享目录与 SSH 接入步骤、构建目录约定；
5. ODR 实验的结论（声明 vs 定义、`inline` 的作用）。

## 验收标准

- [ ] 在虚拟机里从零执行 `cmake -S ... -B ~/build/base && cmake --build` 成功，运行输出正确；
- [ ] 能不看资料讲清四个阶段，以及「头文件里为什么不能随便定义普通函数」；
- [ ] `git log` 显示当天提交，`git status` 干净；
- [ ] `docs/toolchain.md` 和 `note.md` 内容完整，含真实命令输出（可粘贴片段）。

## 自测问题（口头回答，答不上就回查）

1. `g++ -c foo.cpp` 和 `g++ foo.cpp -o foo` 做的事差在哪？
2. `.o` 文件中 `T` 和 `U` 符号分别代表什么？
3. 链接错误 `undefined reference` 和 `multiple definition` 分别通常由什么引起？
4. `#pragma once` 能防止同一个头文件在同一 TU 里被包含两次，为什么防不住 ODR 冲突？
5. `inline` 在 C++ 里除了「建议内联」，更重要的语言层含义是什么？
6. 为什么构建目录不建议放在 VirtualBox 共享文件夹？
7. `g++ -E` 的产物里为什么看不到 `#include` 了？

## 完成后

告诉我「D01 完成」，并附上：

- `cmake --build` 的完整输出；
- `note.md` 和 `docs/toolchain.md`；
- `git log --oneline` 结果。

我会检查工程结构、警告是否清零、笔记里有没有把「声明/定义」真正讲清楚，再放行 D02。
