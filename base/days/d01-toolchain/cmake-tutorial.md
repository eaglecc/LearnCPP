# CMake 完整教程（D01 版）

> 目标：从零理解 CMake 的心智模型，能独立写出「多目标 + 严格警告 + C++20」的工程，
> 并解释每条命令背后真正发生了什么。
> 
> 环境：Ubuntu 24.04（WSL2）/ GCC 13.3 / CMake 3.28 / Ninja。
> 本文所有命令都以本仓库的实际路径为例，可直接复制。

---

## 目录

1. [CMake 到底是什么](#1-cmake-到底是什么)
2. [命令行参数与生成器](#2-命令行参数与生成器)
3. [安装与版本](#3-安装与版本)
4. [最小工程：三行 CMakeLists](#4-最小工程三行-cmakelists)
5. [核心心智模型：Target](#5-核心心智模型target)
6. [源码目录与构建目录](#6-源码目录与构建目录)
7. [语言标准与编译选项](#7-语言标准与编译选项)
8. [多源文件与多目标](#8-多源文件与多目标)
9. [库：add_library 与链接](#9-库add_library-与链接)
10. [构建类型与配置相关选项](#10-构建类型与配置相关选项)
11. [生成配置头 configure_file](#11-生成配置头-configure_file)
12. [依赖管理：find_package](#12-依赖管理find_package)
13. [排错手册](#13-排错手册)
14. [命令速查表](#14-命令速查表)
15. [WSL 专属注意事项](#15-wsl-专属注意事项)
16. [配套练习](#16-配套练习)
17. [进阶预览（D7 之后）](#17-进阶预览d7-之后)

---

## 1. CMake 到底是什么

### 1.1 它不负责编译

一个常见的误解：`cmake` 会去编译代码。**不会。** CMake 全部工作是在"生成另一个构建系统"。

```
  你写的                    CMake 干的活                         真正干活的
┌──────────────┐   configure   ┌──────────────────┐   build   ┌──────────────┐
│ CMakeLists   │──────────────>│ build/build.ninja│──────────>│  g++ / ar / ld│
│   .txt       │   generate    │ build/Makefile   │  (ninja)  │  → .o → 可执行 │
└──────────────┘               └──────────────────┘           └──────────────┘
        ▲                              ▲
        │ 探测编译器、生成缓存            │ 依赖图（谁依赖谁、变了要重编什么）
        └──── build/CMakeCache.txt ────┘
```

三个阶段，对应三条命令：

| 阶段        | 命令                          | 产物                         | 什么时候要重跑           |
| --------- | --------------------------- | -------------------------- | ----------------- |
| configure | `cmake -S <src> -B <build>` | `CMakeCache.txt`           | 首次、换编译器、改 `-D` 选项 |
| generate  | 同上（自动）                      | `build.ninja` / `Makefile` | 同上                |
| build     | `cmake --build <build>`     | `.o`、可执行文件                 | 每次改代码             |

`cmake --build` 是个**统一封装**：背后调 `ninja`（或 `make`）。用它而不是直接 `ninja`，是为了脚本在 Windows/MSVC 上也能跑。

### 1.2 为什么不直接手写 Makefile

|          | 手写 Makefile                  | CMake                                                |
| -------- | ---------------------------- | ---------------------------------------------------- |
| 头文件依赖    | 要自己扫 `#include` 或写 `-MMD` 规则 | 编译器依赖文件自动生成并纳入                                       |
| 平台差异     | Linux/macOS/Windows 各写一套     | 一套脚本，生成不同后端                                          |
| IDE      | 无                            | 能生成 VS 工程、供 VS Code/clangd 读 `compile_commands.json` |
| 库/包查找    | 手写 `-I -L -l` 路径             | `find_package`                                       |
| 测试/安装/打包 | 另写脚本                         | `ctest` / `install()` / `CPack`                      |

代价是 CMake 有自己的一套"语言"（变量、作用域、属性、生成器表达式），学习曲线陡。但**你只需要掌握 20% 就能覆盖 90% 场景**，本文就是这个 20%。

### 1.3 CMake 语言的三个怪异之处

提前知道能少踩坑：

1. **所有值都是字符串**。`set(X 1 2 3)` 得到的是列表 `"1;2;3"`，不是数组。
2. **变量有作用域**。函数、目录、target 各有各的作用域；`add_compile_options` 只影响**它之后**创建的 target。
3. **变量 vs 属性**。`CMAKE_CXX_STANDARD` 是变量（创建 target 时读取一次并固化成属性）；`set_target_properties(... CXX_STANDARD 20)` 直接改属性。改晚了的变量不会追溯影响已存在的 target。

---

## 2. 命令行参数与生成器

### 2.1 同一个程序，两个模式

`cmake` 这同一个可执行文件有两种运行模式，靠有没有 `--build` 区分：

| 模式 | 命令形态 | 干什么 | 频率 |
| --- | --- | --- | --- |
| 配置模式 | `cmake -S ... -B ... -G ... -D...` | 读 CMakeLists.txt、探测编译器、**生成** `build.ninja` | 很少（首次、改 CMakeLists、改 `-D`） |
| 构建模式 | `cmake --build <build>` | 调用底层 ninja/make，**执行**编译 | 每次改代码 |

### 2.2 参数速查

| 参数 | 含义 | 属于哪个阶段 | 说明 |
| --- | --- | --- | --- |
| `-S <dir>` | **S**ource | 配置 | 含 `CMakeLists.txt` 的目录；不写时默认当前目录 |
| `-B <dir>` | **B**uild | 配置 | 产物目录，**不存在会自动创建** |
| `-G <name>` | **G**enerator | 配置 | 生成哪种构建文件，见 2.3 |
| `-D<VAR>=<value>` | cache variable | 配置 | 设置缓存变量，写进 `CMakeCache.txt` |
| `-LAH` | List / Advanced / Help | 配置后 | 列出所有缓存变量及注释 |
| `--build <dir>` | 切换到构建模式 | 构建 | 后面接**构建目录**，不是源码目录 |
| `--target <name>` | 只构建某个 target | 构建 | 配合 `--build` 使用 |
| `-j <N>` | 并行任务数 | 构建 | 透传给底层 ninja/make |
| `-v` / `--verbose` | 打印完整命令行 | 构建 | 验证选项有没有传下去 |
| `--config <type>` | 多配置生成器专用 | 构建 | VS / Ninja Multi-Config 才需要 |
| `--fresh` | 清空缓存重新配置 | 配置 | 3.24+，等价于删 `CMakeCache.txt` |
| `--install <dir>` | 执行安装规则 | 安装 | 对应 `install()` 命令 |

几个容易混的细节：

- `-S` / `-B` / `-G` / `-D` **只在配置期有意义**，`cmake --build` 不接受 `-D`。
- 相对路径相对于**你敲命令时所在的目录**，不是相对于 CMakeLists。
- `-B` 后面跟的是**目录**，不是文件；`-S` 和 `-B` 的**顺序无所谓**。

新旧写法对照：

```bash
# 2018 年以前的写法：要 cd 进构建目录
mkdir build && cd build && cmake .. && make

# 现在：一条命令，不用 cd（CMake 3.13+）
cmake -S . -B build && cmake --build build
```

### 2.3 `-G`：生成器

`-G` 决定"生成什么格式的构建文件"。`cmake --help` 会列出本机可用的生成器（本机实测）：

```
* Unix Makefiles     = Generates standard UNIX makefiles.       ← * 是默认
  Ninja              = Generates build.ninja files.
  Ninja Multi-Config = Generates build-<Config>.ninja files.
  （其余是 CodeBlocks / Eclipse / Kate 等 IDE 工程文件，已废弃）
```

- `-G "Unix Makefiles"` → 生成 `Makefile`，用 `make` 执行
- `-G Ninja` → 生成 `build.ninja`，用 `ninja` 执行
- `-G "Ninja Multi-Config"` → 一个构建目录同时容纳 Debug/Release

**生成器一旦选定就固定在构建目录里**，记录在缓存中：

```
$ grep CMAKE_GENERATOR ~/build/base/CMakeCache.txt
CMAKE_GENERATOR:INTERNAL=Ninja
```

所以同一个构建目录**不能中途换 `-G`**，会直接报错；想换就删掉重建。

### 2.4 Ninja 是什么

**Ninja 是一个构建工具（build system），和 `make` 同类**，不是编译器，也不是 CMake 的替代品。

```
CMakeLists.txt  --cmake-->  build.ninja  --ninja-->  g++ -c ... / g++ ... -o app
   (你写)        (配置期)     (生成物)     (构建期)        (真正编译)
```

Ninja 只管三件事：**读依赖图、判断哪些文件过期、按并行把命令发出去**。编译本身永远是 `g++`/`clang++` 干的。

| | GNU Make | Ninja |
| --- | --- | --- |
| 设计对象 | **人**手写的 Makefile | **程序**生成的构建文件 |
| 语法 | 强大但复杂（函数、隐式规则、变量展开） | 极简，几乎没有"语言" |
| 启动速度 | 解析 Makefile 有开销 | 毫秒级，几乎无解析成本 |
| 增量检查 | 每次重新解析整个 makefile | 专用的 `.ninja_deps` 依赖日志 |
| 默认并行 | 串行，必须 `-j` | **默认用满所有核** |
| 典型场景 | 手写小工程 | CMake/Meson 等大型工程的产物 |

Ninja 是 Google 为 Chromium 写的，核心洞察只有一句：**构建文件反正是程序生成的、人不会去读，那语法就该尽可能简单，让机器解析得快**。所以在"CMake 生成 + 大量源文件"场景下，它比 Make 快得多——大工程光"判断哪些文件要重编"就能差出几秒。

绕过 CMake 直接用 ninja 也行（日常仍推荐 `cmake --build`，跨平台脚本不用改）：

```bash
cd ~/build/base
ninja                 # 构建全部
ninja d01-greet       # 构建某个 target
ninja -v              # 打印命令
ninja -t targets      # 列出所有 target
ninja -t commands     # 列出所有命令
ninja -t clean        # 清理
```

### 2.5 三组容易混的概念

| 容易混 | 区别 |
| --- | --- |
| `-B` vs `--build` | `-B` 是"配置时把产物写到哪"；`--build` 是"现在开始构建，构建目录是哪个"。长得像，作用完全不同 |
| `-G Ninja` vs `ninja` | 前者告诉 CMake "生成 Ninja 格式的文件"；后者拿这些文件去干活 |
| `-D` vs `-DCMAKE_*` | 没有区别，`-D` 是"定义缓存变量"的语法，`CMAKE_BUILD_TYPE` 只是恰好叫这个名字 |

> ⚠️ 常见事故：在源码目录敲 `cmake .`（原地构建），会把 `CMakeCache.txt`、`build.ninja`、`.o` 全塞进仓库，之后想换配置就满地狼藉。这就是为什么永远写 `-B`，指向独立的构建目录。

---

## 3. 安装与版本

```bash
sudo apt update
sudo apt install -y cmake ninja-build build-essential gdb

cmake --version     # cmake version 3.28.3
ninja --version     # 1.11.1
g++ --version       # g++ (Ubuntu 13.3.0-6ubuntu2~24.04) 13.3.0
```

本教程要求 **CMake ≥ 3.28**（task.md 的约定）。版本更老时某些语法会报错，例如：

- `cmake --fresh`：3.24+
- `target_link_options`：3.13+
- `FILE_SET`：3.23+
- `cmake_path`：3.20+

查某个命令从哪个版本开始支持：`cmake --help-command <command>`，底部会写 `Added in version x.y`。

---

## 4. 最小工程：三行 CMakeLists

在本仓库的 `base/CMakeLists.txt` 里写：

```cmake
cmake_minimum_required(VERSION 3.28)
project(CPPLearn LANGUAGES CXX)
add_executable(d01-hello days/d01-toolchain/hello.cpp)
```

### 4.1 逐行解释

**`cmake_minimum_required(VERSION 3.28)`**

两件事：

1. 低于 3.28 的 CMake 直接报错退出；
2. **设定策略版本（policy version）**——决定 CMake 对某些旧行为采取旧规则还是新规则。这就是为什么它必须是第一条命令。

> 不要为了"兼容老机器"把它写成 3.5。那会让 CMake 用 2016 年的语义解释你的脚本，新版特性可能不生效。

**`project(CPPLearn LANGUAGES CXX)`**

- 给工程命名（影响默认产物目录、`CPACK`、IDE 工程名）；
- **探测并启用语言编译器**。`LANGUAGES CXX` 表示"我只用 C++"。
- 不写 `LANGUAGES` 时默认 `C CXX`，会额外去探测 C 编译器；再大的工程还会探测 Fortran/CUDA，浪费时间。

它还会定义一批变量：`PROJECT_NAME`、`PROJECT_SOURCE_DIR`、`CMAKE_PROJECT_NAME`、`CMAKE_CXX_COMPILER` 等。

**`add_executable(d01-hello days/d01-toolchain/hello.cpp)`**

定义一个**可执行目标**：名字 `d01-hello`，源文件是 `hello.cpp`。相对路径相对于**当前 CMakeLists.txt 所在目录**（即 `base/`），不是相对于你在哪个 shell 目录。

### 4.2 跑起来

```bash
cmake -S /mnt/d/Project/CPP/CPPLearn/base -B ~/build/base -G Ninja
cmake --build ~/build/base
~/build/base/d01-hello
```

输出：

```
-- The CXX compiler identification is GNU 13.3.0
-- Detecting CXX compiler ABI info
-- Detecting CXX compile features
-- Configuring done (0.6s)
-- Generating done (0.0s)
-- Build files have been written to: /home/chengyuchao/build/base
Hello, World!
```

`-G Ninja` 指定生成器。不写则 Linux 默认 `Unix Makefiles`。**同一个构建目录不能换生成器**，换了要删掉重建。

生成的目录长这样：

```
~/build/base/
├── CMakeCache.txt          ← 缓存：编译器路径、所有 -D 选项、探测结果
├── CMakeFiles/             ← 内部规则、编译器探测产物
├── build.ninja             ← 真正的构建文件（你的"Makefile"）
├── .ninja_deps / .ninja_log
└── d01-hello               ← 可执行文件
```

**`CMakeCache.txt` 是万恶之源**：一旦它记录了错误的值（比如换了编译器、搬了目录），后续行为会很诡异。终极解法永远是 `rm -rf ~/build/base`。

---

## 5. 核心心智模型：Target

### 5.1 一切围绕 target

CMake 的世界里只有两种东西：**target** 和 **target 的属性/关系**。

```
target: d01-hello
├── 类型: EXECUTABLE
├── 源文件: [hello.cpp]
├── 编译选项: [-Wall -Wextra -Wpedantic -Wconversion]
├── C++ 标准: 20
├── include 目录: [...]
└── 链接库: [greet, Threads::Threads]
```

三种 target：

| 命令                                                  | 类型  | 产物                         |
| --------------------------------------------------- | --- | -------------------------- |
| `add_executable(name ...)`                          | 可执行 | `name`（Linux 无后缀）          |
| `add_library(name [STATIC\|SHARED\|INTERFACE] ...)` | 库   | `libname.a` / `libname.so` |
| `add_custom_target(name ...)`                       | 假目标 | 无产物，只执行命令（如 `make format`） |

**写好 CMake 的秘诀**：先想清楚"我有哪几个 target"，再给每个 target 配属性和依赖。而不是想"我要跑哪些命令"。

### 5.2 属性传播：PRIVATE / PUBLIC / INTERFACE

这是 CMake 最关键、也最容易搞混的概念。以 `target_link_libraries` 为例：

```cmake
target_link_libraries(d01-greet PRIVATE greet)
```

| 关键字         | 含义                 | 谁用                    |
| ----------- | ------------------ | --------------------- |
| `PRIVATE`   | 我自己需要，但**不**对外暴露   | 可执行文件链接库 → 用 PRIVATE  |
| `PUBLIC`    | 我需要，**而且**依赖我的人也需要 | 库 A 用了库 B 的类型（头文件里出现） |
| `INTERFACE` | 我自己不编，但**依赖我的人**需要 | header-only 库         |

一句话判据：**这个依赖是否出现在我的公开头文件里？**

- 出现在 → `PUBLIC`（传给下游）
- 只出现在 `.cpp` 里 → `PRIVATE`
- 我根本没有 `.cpp`（纯头文件库） → `INTERFACE`

这条规则对 `include_directories`、`compile_options`、`compile_definitions`、`link_libraries` 全部适用。

### 5.3 变量式 vs 目标式，优先用目标式

```cmake
# ❌ 全局式：污染本目录及之后所有 target
include_directories(include)
add_compile_options(-Wall)
link_libraries(greet)

# ✅ 目标式：只作用于这个 target，属性清晰
target_include_directories(d01-greet PRIVATE include)
target_compile_options(d01-greet PRIVATE -Wall)
target_link_libraries(d01-greet PRIVATE greet)
```

全局式的坑：它只影响**之后创建**的 target；如果你先 `add_executable` 再 `add_compile_options`，前面那个 target 拿不到选项。这种"顺序敏感"的 bug 极难查。目标式没有这个问题。

---

## 6. 源码目录与构建目录

### 6.1 必须分离（out-of-source build）

```bash
# ✅ 正确
cmake -S /mnt/d/Project/CPP/CPPLearn/base -B ~/build/base

# ❌ 原地构建：把 build.ninja、CMakeCache.txt、.o 全塞进源码目录
cd /mnt/d/Project/CPP/CPPLearn/base && cmake .
```

`-S` = source（有 CMakeLists.txt 的目录），`-B` = build（产物目录）。**永远别省**。

原地构建的后果：源码目录被污染、`git status` 一堆产物、换配置要手动清理、同一份源码没法同时有 Debug 和 Release 两套产物。对比：

```bash
cmake -S . -B ~/build/debug   -DCMAKE_BUILD_TYPE=Debug
cmake -S . -B ~/build/release -DCMAKE_BUILD_TYPE=Release
```

一份源码，两套产物，互不干扰。

### 6.2 本仓库的约定

| 角色    | 路径                                                       | 文件系统        |
| ----- | -------------------------------------------------------- | ----------- |
| 源码、笔记 | `/mnt/d/Project/CPP/CPPLearn`（`D:\Project\CPP\CPPLearn`） | 9p（跨 VM）    |
| 构建产物  | `~/build/base`                                           | ext4（VM 原生） |

**为什么构建目录绝不能放 `/mnt/d`**：见第 15 节，这里先说结论——会直接报 `Operation not permitted`。

---

## 7. 语言标准与编译选项

### 7.1 C++ 标准：三件套

```cmake
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
```

| 变量                      | 作用        | 不设的后果                                |
| ----------------------- | --------- | ------------------------------------ |
| `CMAKE_CXX_STANDARD 20` | 至少 C++20  | 默认用老标准（GCC 13 默认 gnu++17）            |
| `..._REQUIRED ON`       | 必须满足，否则报错 | 编译器不支持时静默降级到低版本                      |
| `..._EXTENSIONS OFF`    | 关闭 GNU 扩展 | 生成 `-std=gnu++20` 而非严格的 `-std=c++20` |

三条必须写在 `add_executable` / `add_library` **之前**——它们在创建 target 时被读取并固化为 `CXX_STANDARD` 属性。

目标式等价写法（更适合库）：

```cmake
set_target_properties(d01-hello PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    CXX_EXTENSIONS OFF)
# 或者（推荐，表达"我需要哪些特性"而不是"哪个标准号"）
target_compile_features(d01-hello PRIVATE cxx_std_20)
```

### 7.2 警告选项

```cmake
add_compile_options(-Wall -Wextra -Wpedantic -Wconversion)
```

| 选项             | 作用                                       |
| -------------- | ---------------------------------------- |
| `-Wall`        | 常用警告集合                                   |
| `-Wextra`      | `-Wall` 之外的额外警告                          |
| `-Wpedantic`   | 严格 ISO C++，拒绝编译器扩展                       |
| `-Wconversion` | 隐式数值转换（`double → int`、`size_t → int`）会警告 |

`-Wconversion` 是最吵的一个，也是最能抓出真实 bug 的。新手常见触发点：

```cpp
int n = v.size();                       // size_t → int，警告
int n = static_cast<int>(v.size());     // 显式转换，正确
double d = 3.14; int i = d;             // 警告
```

**它和 D01 的目标一致**：让编译器在编译期替你抓问题，而不是等到运行时核心转储。

`add_compile_options` 是目录作用域，对**之后创建**的所有 target 生效。想只对一个 target 加：

```cmake
target_compile_options(d01-hello PRIVATE -Wall -Wextra -Wpedantic -Wconversion)
```

更工程化的写法（D7 会讲）：定义一个函数或 INTERFACE 库 `project_warnings`，谁需要谁链接。

### 7.3 验证选项真的传下去了

```bash
cmake --build ~/build/base -v
```

输出里每条编译命令应该长这样：

```
/usr/bin/c++ -I... -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -o CMakeFiles/d01-hello.dir/days/d01-toolchain/hello.cpp.o -c /mnt/d/.../hello.cpp
```

`-std=c++20` 来自 `CMAKE_CXX_STANDARD`，警告来自 `add_compile_options`，`-I...` 来自 `target_include_directories`。**这条命令就是 CMake 的全部产出**：把声明翻译成命令行。

用 Ninja 时还能看完整依赖图：

```bash
cd ~/build/base
ninja -t commands d01-hello   # 构建这个 target 的所有命令
ninja -t deps                 # 依赖关系
ninja -t graph d01-hello | dot -Tpng > graph.png   # 依赖图（需要 graphviz）
```

### 7.4 默认的构建类型是空的

`CMAKE_BUILD_TYPE` 不设置时，编译命令里**既没有 `-O2` 也没有 `-g`**（GCC 默认 `-O0` 且无调试信息）。养成习惯：

```cmake
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
                 Debug Release RelWithDebInfo MinSizeRel)
endif()
```

---

## 8. 多源文件与多目标

### 8.1 一个 target，多个源文件

```cmake
add_executable(d01-greet
    days/d01-toolchain/code/main.cpp
    days/d01-toolchain/code/greet.cpp
)
```

CMake 会对每个 `.cpp` 生成一条编译命令，再统一链接——这正是你任务 3 手工做的四步。**头文件不要写进列表**（它不产生 `.o`），CMake 通过编译器生成的依赖文件自动追踪 `#include`。

### 8.2 每个 `main` 必须独立成 target

一个可执行文件只能有一个 `main`。所以：

```cmake
add_executable(d01-hello days/d01-toolchain/hello.cpp)
add_executable(d01-greet days/d01-toolchain/code/main.cpp
                          days/d01-toolchain/code/greet.cpp)
# odr_a.cpp 带 main，odr_b.cpp 不带 —— 它们组合起来是"故意链接失败"的演示，
# 不要做成 target，否则 cmake --build 永远失败。
```

如果你看到：

```
multiple definition of `main'; CMakeFiles/xxx.dir/yyy.cpp.o: first defined here
```

就是这个原因——两个带 `main` 的文件进了同一个 target。

### 8.3 不要用 GLOB

```cmake
# ❌ 反模式
file(GLOB SOURCES "days/d01-toolchain/code/*.cpp")
add_executable(d01-greet ${SOURCES})
```

三个问题：

1. **新增文件不会触发重新 configure**。你新建一个 `.cpp`，`cmake --build` 发现不了，得手动重跑 `cmake`。团队协作时是灾难。
2. 它会把 `odr_a.cpp`、`odr_b.cpp` 也卷进来 → `multiple definition of main`。
3. 隐式依赖，读 CMakeLists 看不出工程里有哪些文件。

**显式列文件名**。麻烦，但可靠。

---

## 9. 库：add_library 与链接

当同一份代码被多个 target 使用时，抽成库。

### 9.1 静态库

```cmake
add_library(greet STATIC
    days/d01-toolchain/code/greet.cpp
)
target_include_directories(greet PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/days/d01-toolchain/code
)
target_compile_options(greet PRIVATE -Wall -Wextra -Wpedantic -Wconversion)

add_executable(d01-greet days/d01-toolchain/code/main.cpp)
target_link_libraries(d01-greet PRIVATE greet)
```

产物是 `libgreet.a`（归档文件，链接时被拷进可执行文件）。**注意 `PUBLIC`**：因为 `main.cpp` 里要 `#include "greet.h"`，头文件目录必须传给消费者。

### 9.2 静态 vs 共享

|       | `STATIC`  | `SHARED`                            |
| ----- | --------- | ----------------------------------- |
| 产物    | `libx.a`  | `libx.so`                           |
| 链接方式  | 代码拷进可执行文件 | 运行时动态加载                             |
| 运行时依赖 | 无         | 需要 `.so` 在 `LD_LIBRARY_PATH` 或系统路径里 |
| 体积    | 可执行文件大    | 可执行文件小，库共享                          |
| 改库后   | 要重新链接     | 替换 `.so` 即可（ABI 兼容时）                |

共享库必须用位置无关代码（PIC），CMake 自动加 `-fPIC`。全局开关：

```cmake
option(BUILD_SHARED_LIBS "Build shared libraries" OFF)
```

### 9.3 INTERFACE 库：只带头文件

```cmake
add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion)

# 任何 target 链接它，就自动获得这些警告选项
target_link_libraries(d01-hello PRIVATE project_warnings)
target_link_libraries(d01-greet PRIVATE project_warnings)
```

INTERFACE 库不产生任何编译产物，只**携带使用要求**（usage requirements）。这是现代 CMake 里复用编译选项的标准做法。

---

## 10. 构建类型与配置相关选项

### 10.1 四种标准构建类型

| 类型               | 编译选项              | 用途                              |
| ---------------- | ----------------- | ------------------------------- |
| `Debug`          | `-g -O0`          | 调试（GDB 需要 `-g`）                 |
| `Release`        | `-O3 -DNDEBUG`    | 发布                              |
| `RelWithDebInfo` | `-O2 -g -DNDEBUG` | 性能分析（`perf`/`valgrind` 要看得到函数名） |
| `MinSizeRel`     | `-Os -DNDEBUG`    | 体积优先                            |

```bash
cmake -S ... -B ~/build/base -G Ninja -DCMAKE_BUILD_TYPE=Debug
```

`-D` 是设置**缓存变量**（会写进 `CMakeCache.txt`）。`CMAKE_BUILD_TYPE` 只在**单配置生成器**（Ninja、Makefiles）下有效；多配置生成器（Visual Studio、`Ninja Multi-Config`）用 `cmake --build build --config Release`。

查看所有缓存变量：

```bash
cmake -LAH ~/build/base          # L=列表, A=包含高级选项, H=带注释
grep CMAKE_BUILD_TYPE ~/build/base/CMakeCache.txt
```

### 10.2 按配置追加选项

```cmake
target_compile_options(d01-hello PRIVATE
    $<$<CONFIG:Debug>:-g;-O0>
    $<$<CONFIG:Release>:-O2>
)
```

`$<...>` 是**生成器表达式**（generator expression），在 generate 阶段才求值。语法读作"If CONFIG is Debug then `-g -O0`"。

### 10.3 Sanitizer 开关（D7 起常用）

```cmake
option(ENABLE_SANITIZERS "Build with ASan + UBSan" OFF)
if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()
```

用法：

```bash
cmake -S ... -B ~/build/asan -G Ninja -DCMAKE_BUILD_TYPE=Debug -DENABLE_SANITIZERS=ON
cmake --build ~/build/asan
~/build/asan/d01-greet
```

> `-fno-omit-frame-pointer` 让栈回溯完整；`add_link_options` 不能少，否则链接期符号解析失败。

线程检查（TSan）不能和 ASan 同时开，单独建一个构建目录：

```bash
cmake -S ... -B ~/build/tsan -DCMAKE_BUILD_TYPE=Debug \
      -DCMAKE_CXX_FLAGS="-fsanitize=thread -g"
```

---

## 11. 生成配置头 configure_file

把 CMake 里的信息（版本号、宏开关）写进 C++ 头文件。

`cmake/config.h.in`：

```c
#pragma once
#define PROJECT_NAME "@PROJECT_NAME@"
#define PROJECT_VERSION "@PROJECT_VERSION@"
#cmakedefine01 ENABLE_SANITIZERS
```

`CMakeLists.txt`：

```cmake
project(CPPLearn VERSION 0.1.0 LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
configure_file(cmake/config.h.in ${CMAKE_BINARY_DIR}/generated/config.h @ONLY)
target_include_directories(d01-hello PRIVATE ${CMAKE_BINARY_DIR}/generated)
```

生成结果（`~/build/base/generated/config.h`）：

```c
#pragma once
#define PROJECT_NAME "CPPLearn"
#define PROJECT_VERSION "0.1.0"
#define ENABLE_SANITIZERS 0
```

**规则**：生成文件永远放在 `${CMAKE_BINARY_DIR}`，不要写回源码目录。

---

## 12. 依赖管理：find_package

系统里已有的库，用 `find_package` 找，然后用它的**导入目标**链接。

```cmake
find_package(Threads REQUIRED)
target_link_libraries(my_server PRIVATE Threads::Threads)
```

```cmake
find_package(fmt CONFIG REQUIRED)
target_link_libraries(d01-greet PRIVATE fmt::fmt)
```

两种模式：

| 模式     | 命令写法                     | 依赖什么                            |
| ------ | ------------------------ | ------------------------------- |
| MODULE | `find_package(X)`        | CMake 自带的 `FindX.cmake` 或系统路径探测 |
| CONFIG | `find_package(X CONFIG)` | 库自己安装的 `XConfig.cmake`          |

现代库（用 CMake 构建的那些）都提供 CONFIG 模式，并且导出 `Namespace::Target` 形式的导入目标——它自带 include 目录、编译选项、传递依赖，`target_link_libraries` 一行搞定，不用再手动写 `-I/-L/-l`。

安装开发包：

```bash
sudo apt install libfmt-dev libgtest-dev libssl-dev
```

> D7 及以后会用到 `Threads::Threads`（并发）和 GoogleTest。

---

## 13. 排错手册

### 13.1 遇到问题先做这三件事

```bash
# 1. 看完整命令行，确认选项真的传下去了
cmake --build ~/build/base -v

# 2. 看缓存里的实际值
cmake -LAH ~/build/base | grep -i cxx

# 3. 实在不行，从零开始（99% 的诡异问题都消失）
rm -rf ~/build/base && cmake -S ... -B ~/build/base -G Ninja
```

### 13.2 常见错误对照表

| 报错                                                                                                              | 原因                                    | 解决                                    |
| --------------------------------------------------------------------------------------------------------------- | ------------------------------------- | ------------------------------------- |
| `The source directory "... " does not appear to contain CMakeLists.txt`                                         | `-S` 指错目录                             | 指向含有 `CMakeLists.txt` 的目录             |
| `The current CMakeCache.txt directory ... is different than the directory ... where CMakeCache.txt was created` | 构建目录被移动/复制过                           | `rm -rf build` 重来                     |
| `configure_file: Operation not permitted`                                                                       | **构建目录在 9p（`/mnt/...`）上**，不支持 `chmod` | 构建目录放 `~/build`，见第 15 节               |
| `No CMAKE_CXX_COMPILER could be found`                                                                          | 没装 `build-essential`                  | `sudo apt install build-essential`    |
| `multiple definition of 'main'`                                                                                 | 两个带 `main` 的源文件在同一 target             | 拆成多个 `add_executable`                 |
| `undefined reference to 'greet()'`                                                                              | 漏了 `greet.cpp`，或只列了头文件                | 把实现文件加进 target                        |
| `Cannot find source file: xxx.cpp`                                                                              | 路径相对的是 CMakeLists.txt 所在目录            | 检查相对路径                                |
| `Target "x" links to target "y" but the target was not found`                                                   | `find_package` 没调用或库名写错               | 补 `find_package` 或检查目标名               |
| `Manually-specified variables were not used`                                                                    | `-D` 变量名拼错/没被引用                       | 检查拼写                                  |
| `CMake Warning ... CMP00xx`                                                                                     | 策略未设置                                 | 提高 `cmake_minimum_required` 版本        |
| 改了 `CMAKE_CXX_STANDARD` 没效果                                                                                     | target 已经创建了，变量不再追溯                   | 把 `set()` 移到 `add_executable` 之前，或删缓存 |

### 13.3 打印调试

```cmake
message(STATUS "SOURCE_DIR = ${CMAKE_CURRENT_SOURCE_DIR}")
message(WARNING "BUILD_TYPE = '${CMAKE_BUILD_TYPE}'")
message(FATAL_ERROR "stop here")
```

`STATUS` 输出到 stdout，`WARNING`/`FATAL_ERROR` 到 stderr。想追踪每条命令的求值过程：

```bash
cmake --trace-expand -S ... -B ... 2>&1 | head -100
```

输出极其啰嗦，只在语法/变量纠结时用。

---

## 14. 命令速查表

> 每个参数的含义、`-G` 与 Ninja 的关系，见第 2 章。

```bash
# ---- 配置 ----
cmake -S <src> -B <build>                 # 配置（默认生成器）
cmake -S <src> -B <build> -G Ninja        # 指定 Ninja
cmake -S <src> -B <build> -DCMAKE_BUILD_TYPE=Debug
cmake -S <src> -B <build> -DMY_OPTION=ON
cmake --fresh -S <src> -B <build>         # 3.24+：清空缓存重配

# ---- 构建 ----
cmake --build <build>                     # 构建全部
cmake --build <build> -j$(nproc)          # 并行
cmake --build <build> --target d01-hello  # 只构建某个 target
cmake --build <build> -v                  # 打印完整命令
cmake --build <build> --clean-first       # 先 clean

# ---- 查看 ----
cmake -LAH <build>                        # 所有缓存变量（含注释）
grep CMAKE_BUILD_TYPE <build>/CMakeCache.txt
cmake --build <build> --target help       # 列出所有可构建目标

# ---- 清理 ----
rm -rf <build>                            # 终极清理，永远有效
cmake --build <build> --target clean      # 只删产物

# ---- 运行 ----
./<build>/<target-name>
```

> **日常循环只有一条**：`cmake --build ~/build/base`。
> 只要不改 `CMakeLists.txt`、不改 `-D` 选项，就不需要重新 configure——Ninja 自己会发现 `CMakeLists.txt` 变了并触发重配（输出里会打印 `Re-running CMake...`）。

---

## 15. WSL 专属注意事项

### 15.1 构建目录必须在 ext4（`~/`），不能在 `/mnt/*`

实际会看到的报错：

```
CMake Error at /usr/share/cmake-3.28/Modules/CMakeDetermineSystem.cmake:246 (configure_file):
  Operation not permitted
```

原因：`/mnt/d` 是 9p 协议挂的 Windows 目录，**默认不支持 Unix 权限位**：

```bash
$ touch /mnt/d/t && chmod 755 /mnt/d/t
chmod: changing permissions of '/mnt/d/t': Operation not permitted
```

而 `project()` 里的 `configure_file` 需要拷贝文件并设置权限 → 失败。

修复：

```bash
# 1. 构建目录放到 ext4
cmake -S /mnt/d/Project/CPP/CPPLearn/base -B ~/build/base -G Ninja

# 2. 可选：让 /mnt/d 支持 metadata（改完要重启 WSL）
#    /etc/wsl.conf 追加：
#      [automount]
#      options = "metadata,umask=22,fmask=11"
#    然后 Windows 侧执行： wsl --shutdown
```

即使加了 `metadata`，**构建目录仍然要放 `~/build`**：9p 每次文件操作都要跨虚拟机往返，大量小文件（`.o`、头文件）会慢几倍。

### 15.2 源码在 `/mnt/d` 是可以的

编译器**读**源码走 9p 没问题，只有**写**构建产物需要 ext4。所以"源码在 Windows、构建在 Linux"这个组合完全成立，也是本仓库采用的方案。

如果工程变大（几百个源文件），可以把源码也搬进 `~/`，再用 `\\wsl.localhost` 或 git 同步回 Windows。

### 15.3 其他

| 现象                                     | 原因                   | 处理                                                   |
| -------------------------------------- | -------------------- | ---------------------------------------------------- |
| `git` 报 `mode change 100644 => 100755` | `/mnt/d` 无权限位        | 加 `metadata` 选项，或 `git config core.fileMode false`   |
| 脚本 `bad interpreter: /bin/bash^M`      | Windows 编辑器写入了 CRLF  | `.gitattributes` 设 `* text=auto eol=lf`；`dos2unix` 修 |
| 文件名大小写混淆                               | Windows 不区分、Linux 区分 | 统一用小写下划线命名                                           |
| `perf` 不可用                             | WSL2 拿不到 PMU（和虚拟机一样） | 用 valgrind/callgrind、`gprof`、Google Benchmark        |

---

## 16. 配套练习

按顺序做，每个都要亲手跑过并记录输出。

### 练习 1：最小工程（任务 5 要求）

1. 写 `base/CMakeLists.txt` 三件套：`cmake_minimum_required` / `project` / `add_executable`
2. 配置到 `~/build/base`，构建，运行 `d01-hello`
3. 加 `-v` 看命令行

### 练习 2：标准与警告

1. 补上 `CMAKE_CXX_STANDARD` 三件套和 `add_compile_options`
2. 确认 `cmake --build -v` 里出现 `-std=c++20 -Wall -Wextra -Wpedantic -Wconversion`
3. 故意写一句 `int n = 3.14;`，看 `-Wconversion` 怎么报，再用 `static_cast` 修

### 练习 3：第二个 target

1. 加 `d01-greet`（`main.cpp` + `greet.cpp`）
2. 故意把 `main.cpp` 和 `hello.cpp` 塞进同一个 target，复现 `multiple definition of main`，再改回来
3. `cmake --build ~/build/base --target d01-greet` 只构建一个

### 练习 4：库

1. 把 `greet.cpp` 抽成 `STATIC` 库
2. `target_include_directories(greet PUBLIC ...)`，`target_link_libraries(d01-greet PRIVATE greet)`
3. 观察产物 `libgreet.a`，并用 `nm` 看它的符号（呼应 D01 任务 3）

### 练习 5：选项与配置

1. 加 `option(ENABLE_SANITIZERS ...)`，用 `-DENABLE_SANITIZERS=ON` 构建到另一个目录 `~/build/asan`
2. 写一句越界访问，对比普通构建和 ASan 构建的运行输出
3. 用 `cmake -LAH` 找到你定义的选项

### 练习 6：compile_commands.json

1. `set(CMAKE_EXPORT_COMPILE_COMMANDS ON)`
2. 确认 `~/build/base/compile_commands.json` 存在
3. 读一条记录，指出哪部分是 `-I` 来自哪里、`-std=c++20` 来自哪里

---

## 17. 进阶预览（D7 之后）

本文刻意没展开的部分，知道"有这回事"即可：

| 主题                       | 一句话                                         | 何时学    |
| ------------------------ | ------------------------------------------- | ------ |
| `add_subdirectory`       | 多目录工程，每个子目录一套 CMakeLists                    | 工程变大时  |
| `CMakePresets.json`      | 把 `-D...` 那串参数固化成命名的 preset                 | D7     |
| `ctest` / `add_test`     | 把可执行文件注册成测试，`ctest` 批量跑                     | D7     |
| `install()` / `export()` | 安装规则、导出 `XConfig.cmake` 给别人 `find_package`  | 写库时    |
| `CPack`                  | 打 deb/rpm/zip 包                             | 发布时    |
| `add_custom_command`     | 构建时跑代码生成器（protobuf 等）                       | 有代码生成时 |
| `ccache`                 | `-DCMAKE_CXX_COMPILER_LAUNCHER=ccache`，编译缓存 | 立刻可用   |
| `cmake --build --preset` | preset 驱动构建                                 | D7     |

**马上能用的一个**：

```bash
sudo apt install -y ccache
cmake -S ... -B ~/build/base -G Ninja -DCMAKE_CXX_COMPILER_LAUNCHER=ccache
```

重复编译同样的文件（比如切换分支后）会命中缓存，第二次几乎瞬间完成。

---

## 附录：完整参考工程

```cmake
cmake_minimum_required(VERSION 3.28)

project(CPPLearn VERSION 0.1.0 LANGUAGES CXX)

# ---- 全局设置 ----
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 默认 Debug（单配置生成器）
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(ENABLE_SANITIZERS "Build with ASan + UBSan" OFF)
if(ENABLE_SANITIZERS)
    add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
    add_link_options(-fsanitize=address,undefined)
endif()

# 警告：用 INTERFACE 库统一分发
add_library(project_warnings INTERFACE)
target_compile_options(project_warnings INTERFACE
    -Wall -Wextra -Wpedantic -Wconversion)

# ---- 目标 ----
add_executable(d01-hello
    days/d01-toolchain/hello.cpp
)
target_link_libraries(d01-hello PRIVATE project_warnings)

add_executable(d01-greet
    days/d01-toolchain/code/main.cpp
    days/d01-toolchain/code/greet.cpp
)
target_link_libraries(d01-greet PRIVATE project_warnings)
```

---

## 自测问题

1. `cmake -S . -B build` 生成的是什么？谁真正执行编译？
2. `cmake_minimum_required` 为什么必须是第一行？
3. `CMAKE_CXX_STANDARD` 和 `CMAKE_CXX_STANDARD_REQUIRED` 分别解决什么问题？
4. `PRIVATE` 和 `PUBLIC` 的判据是什么？
5. 为什么 `add_compile_options` 写在 `add_executable` 后面会失效？
6. 为什么不能用 `file(GLOB)` 收集源文件？
7. `CMAKE_BUILD_TYPE` 不设时，编译命令里少了什么？
8. WSL 里为什么构建目录不能放 `/mnt/d`？两个层面的原因分别是什么？
9. `cmake --build` 和 `ninja` 是什么关系？
10. 遇到 CMake 的诡异问题，第一步该做什么？
11. `-B` 和 `--build` 分别服务于哪个模式？
12. Ninja 是编译器吗？它在流水线里处于哪一环？
13. 为什么同一个构建目录不能中途换 `-G`？

答案都在正文里，答不上来就回查对应章节。
