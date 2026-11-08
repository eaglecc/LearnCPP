# D03 任务 6：CMake、笔记与收尾

> 环境：WSL2 Ubuntu 24.04，g++ 13.3.0，CMake 3.28.3，Ninja。
> CMake 文件：[base/CMakeLists.txt](../../CMakeLists.txt)
> 构建目录：`~/build/base`
>
> 本任务把 D03 的 5 个可执行程序接入 CMake，并做一次完整的构建、运行和检查。
> 故意失败的 `fail_*.cpp` 不进入 CMake。

## 0. 速查

```bash
# 配置：-S 源码目录，-B 构建目录
cmake -S /mnt/d/Project/CPP/CPPLearn/base -B ~/build/base

# 只构建 D03 的 5 个 target
cmake --build ~/build/base --target \
    d03-overload d03-defaults d03-namespaces d03-linkage d03-debug \
    -j"$(nproc)"

# 运行
~/build/base/d03-overload
~/build/base/d03-defaults
~/build/base/d03-namespaces
~/build/base/d03-linkage
~/build/base/d03-debug
```

| target | 源文件 | 说明 |
|---|---|---|
| `d03-overload` | `overload.cpp` | 重载解析 |
| `d03-defaults` | `default_args.cpp` | 默认参数 |
| `d03-namespaces` | `namespaces.cpp` | 命名空间与 `using` |
| `d03-linkage` | `link_main.cpp` + `link_stats.cpp` | 多文件链接 |
| `d03-debug` | `debug_me.cpp` | 额外带 `-g -O0`，供 GDB 使用 |

> **记忆点：** 一个带 `main` 的程序对应一个 target；多文件程序把多个 `.cpp` 放进同一个 `add_executable`。

## 1. CMakeLists 里的 D03 部分

```cmake
# D03：每个带 main 的练习文件一个独立 target。
add_executable(d03-overload days/d03-functions-gdb/code/overload.cpp)
target_compile_features(d03-overload PRIVATE cxx_std_20)
target_compile_options(d03-overload PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror)

add_executable(d03-defaults days/d03-functions-gdb/code/default_args.cpp)
target_compile_features(d03-defaults PRIVATE cxx_std_20)
target_compile_options(d03-defaults PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror)

add_executable(d03-namespaces days/d03-functions-gdb/code/namespaces.cpp)
target_compile_features(d03-namespaces PRIVATE cxx_std_20)
target_compile_options(d03-namespaces PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror)

add_executable(d03-linkage
    days/d03-functions-gdb/code/link_main.cpp
    days/d03-functions-gdb/code/link_stats.cpp)
target_compile_features(d03-linkage PRIVATE cxx_std_20)
target_compile_options(d03-linkage PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror)

add_executable(d03-debug days/d03-functions-gdb/code/debug_me.cpp)
target_compile_features(d03-debug PRIVATE cxx_std_20)
target_compile_options(d03-debug PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror
    -g -O0)
```

逐项解释：

| 命令 | 作用 |
|---|---|
| `add_executable(name sources...)` | 定义一个可执行 target，并指定源文件 |
| `target_compile_features(name PRIVATE cxx_std_20)` | 这个 target 用 C++20 |
| `target_compile_options(name PRIVATE flags...)` | 只给这个 target 加编译选项 |
| `PRIVATE` | 选项不向依赖它的 target 传播；这里没有下游 target，写 `PRIVATE` 最直接 |

几个容易混淆的点：

1. **一个 target 只能有一个 `main`**。`d03-linkage` 有两个 `.cpp`，但只有 `link_main.cpp` 里有 `main`，`link_stats.cpp` 只提供 `sum` / `average`，所以能链接成一个程序。
2. **不是把所有 `.cpp` 都塞进一个 target**。`overload.cpp`、`default_args.cpp`、`namespaces.cpp`、`debug_me.cpp` 各自都有 `main`，必须分成独立 target，否则会出现多个 `main` 的定义。
3. **任务 1 里的 `d03-funcs` 已按任务书改名为 `d03-overload`**。功能不变，只是 target 名统一成 `d03-*` 的形式。
4. **`CMAKE_CXX_EXTENSIONS OFF` 决定用 `-std=c++20`**，而不是 `-std=gnu++20`。

> **记忆点：** `add_executable` 定义程序，`target_compile_features` 定标准，`target_compile_options` 定警告；一个 `main` 一个 target。

## 2. 配置与构建

配置：

```bash
cmake -S /mnt/d/Project/CPP/CPPLearn/base -B ~/build/base
```

实际输出：

```text
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: /home/chengyuchao/build/base
```

构建 5 个 target：

```bash
cmake --build ~/build/base --target \
    d03-overload d03-defaults d03-namespaces d03-linkage d03-debug \
    -j"$(nproc)"
```

为了看到完整重建过程，这里用了 `--clean-first`：

```bash
cmake --build ~/build/base --target \
    d03-overload d03-defaults d03-namespaces d03-linkage d03-debug \
    --clean-first -j"$(nproc)"
```

实际输出：

```text
[1/1] Cleaning all built files...
Cleaning... 17 files.
[1/11] Building CXX object CMakeFiles/d03-linkage.dir/days/d03-functions-gdb/code/link_stats.cpp.o
[2/11] Building CXX object CMakeFiles/d03-namespaces.dir/days/d03-functions-gdb/code/namespaces.cpp.o
[3/11] Building CXX object CMakeFiles/d03-overload.dir/days/d03-functions-gdb/code/overload.cpp.o
[4/11] Building CXX object CMakeFiles/d03-linkage.dir/days/d03-functions-gdb/code/link_main.cpp.o
[5/11] Building CXX object CMakeFiles/d03-defaults.dir/days/d03-functions-gdb/code/default_args.cpp.o
[6/11] Building CXX object CMakeFiles/d03-debug.dir/days/d03-functions-gdb/code/debug_me.cpp.o
[7/11] Linking CXX executable d03-overload
[8/11] Linking CXX executable d03-namespaces
[9/11] Linking CXX executable d03-linkage
[10/11] Linking CXX executable d03-defaults
[11/11] Linking CXX executable d03-debug
```

没有 `warning:` 行，说明在 `-Werror` 下 5 个 target 都是零警告。

参数解释：

| 参数 | 作用 |
|---|---|
| `-S` | 源码目录，里面有 `CMakeLists.txt` |
| `-B` | 构建目录，生成的 `build.ninja` 等文件放这里 |
| `--build` | 调用构建工具，不是重新配置 |
| `--target` | 只构建指定 target |
| `-j"$(nproc)"` | 用所有 CPU 核心并行构建 |
| `--clean-first` | 先清理再构建，适合确认“从零开始也能过” |

> **记忆点：** `-S` 指源码、`-B` 指构建目录；`--target` 只构建指定程序，不带 target 才构建所有默认 target。

## 3. 运行结果

```bash
~/build/base/d03-overload
```

```text
== pick ==
pick(int): 1
pick(int): 97
pick(float): 1
pick(double): 1
== touch ==
touch(int&): 1
touch(const int&): 2
touch(const int&): 3
== ptr ==
ptr(nullptr_t)
ptr(int*): 0
```

```bash
~/build/base/d03-defaults
```

```text
log: hello, level=0, flush=false
log: hello, level=1, flush=false
log: hello, level=1, flush=true
```

```bash
~/build/base/d03-namespaces
```

```text
Sum: 7, Double Sum: 14
using declaration: 3
using directive: 5
Hidden counter: 1
Hidden counter: 2
```

```bash
~/build/base/d03-linkage
```

```text
sum=108
average=18
```

`d03-linkage` 是多文件程序的验收重点：`link_main.cpp` 里的 `main` 调用 `link_stats.cpp` 里的 `sum` / `average`，最终链接成一个可执行文件，输出正确。

```bash
~/build/base/d03-debug
```

```text
Sum: 10
Average: 2
```

注意：`d03-debug` 编译的是当前仍带 bug 的 `debug_me.cpp`，循环条件是 `i < n - 1`，所以输出是 `10/2`，不是正确的 `15/3`。这个 target 的任务是“带上 `-g -O0` 方便调试”，逻辑修复见 [note_task4.md](note_task4.md)。

把源码改成：

```cpp
for (int i = 0; i < n; ++i) {
```

重新构建后应得到：

```text
Sum: 15
Average: 3
```

> **记忆点：** `d03-linkage` 的验收输出是 `sum=108`、`average=18`；`d03-debug` 能编译不代表逻辑正确，调试 target 和修 bug 是两件事。

## 4. 验证构建细节

### 4.1 确认 `d03-debug` 真的带 `-g -O0`

不要只看 `CMakeLists.txt`，直接看生成的 `build.ninja`：

```bash
grep -A2 "debug_me.cpp.o:" ~/build/base/build.ninja | grep FLAGS
```

实际输出：

```text
FLAGS = -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror -g -O0
```

说明：

- `-std=c++20` 来自 `target_compile_features(... cxx_std_20)`；
- 警告选项来自 `target_compile_options`；
- `-g -O0` 只加在 `d03-debug` 上，其他 target 没有。

### 4.2 确认失败实验没有进入构建

```bash
grep -c "fail_.*\.cpp" ~/build/base/build.ninja
grep -c "fail_" /mnt/d/Project/CPP/CPPLearn/base/CMakeLists.txt
```

实际输出：

```text
0
0
```

两个都是 0，说明 `fail_*.cpp` 既没有写进 `CMakeLists.txt`，也没有出现在构建图里。这是故意的：它们本来就编译失败或链接失败，加进去会让整个工程构建失败。

> **记忆点：** 验证编译选项看 `build.ninja` 的 `FLAGS`；故意失败的文件只用于手动实验，永远不要加进 CMake。

## 5. CMake 常用命令

| 目的 | 命令 |
|---|---|
| 配置工程 | `cmake -S . -B build` |
| 指定 Ninja | `cmake -S . -B build -G Ninja` |
| 构建所有默认 target | `cmake --build build` |
| 只构建一个 target | `cmake --build build --target d03-debug` |
| 并行构建 | `cmake --build build -j"$(nproc)"` |
| 清理后重建 | `cmake --build build --clean-first` |
| 查看详细命令 | `cmake --build build --verbose` |
| 重新配置 | `cmake -S . -B build` |
| 查看 Ninja target | `ninja -C build -t targets all \| grep d03-` |

日常最常用的就是前四个。

> **记忆点：** `cmake -S/-B` 负责生成构建文件，`cmake --build` 才真正编译；改 `CMakeLists.txt` 后要重新配置。

## 6. D03 收尾检查

| 项目 | 状态 |
|---|---|
| 重载解析、默认参数、命名空间笔记 | `note_task1.md` |
| 链接属性、`static` / 匿名 namespace / `const` / `inline` | `note_task2.md` |
| 多文件编译、`undefined reference`、`multiple definition` | `note_task3.md` |
| GDB 断点、单步、`bt`、`frame`、`watch`、`finish` | `note_task4.md` |
| Git 分支、合并、rebase 与 merge 的区别 | `note_task5.md` |
| CMake 五个 target、零警告构建 | 本篇 |
| `fail_*.cpp` 不进入 CMake | 已验证，构建图中为 0 |
| `d03-debug` 带 `-g -O0` | 已验证，`FLAGS` 中有 `-g -O0` |

D03 的主线可以串成：

```text
函数特性（重载/默认参数）
    ↓
命名空间与链接属性
    ↓
多文件编译、链接错误
    ↓
GDB 调试
    ↓
Git 分支管理
    ↓
CMake 统一构建
```

> **记忆点：** D03 不是六个孤立任务：语言特性最终要经过“编译 → 链接 → 调试 → 版本管理 → 构建系统”这条完整链路。

## 7. 易错点

1. **把多个带 `main` 的 `.cpp` 放进同一个 target**：链接时报 `multiple definition of main`。
2. **把 `fail_*.cpp` 加进 CMake**：构建直接失败；失败实验只能手动编译。
3. **target 名和任务书不一致**：任务书要 `d03-overload`，旧笔记里是 `d03-funcs`；构建时 `--target` 名字要对上。
4. **忘记给 `d03-debug` 加 `-g -O0`**：GDB 里看不到行号或变量。
5. **只改 `CMakeLists.txt` 不重新配置**：`build.ninja` 可能还是旧的；重新运行 `cmake -S ... -B ...`。
6. **以为 `cmake --build` 会重新配置**：它只是调用 Ninja/Make；配置阶段是 `cmake -S/-B`。
7. **在源码目录里直接 `cmake .`**：会把生成文件混进源码；用 `-B` 做 out-of-source 构建。
8. **看到 `d03-debug` 能编译就以为 bug 修好了**：编译成功只说明语法和类型没问题，逻辑错误仍然存在。
9. **忘了 `-Werror` 的含义**：任何警告都会变成错误；零警告构建才算通过。
10. **`d03-linkage` 只编译 `link_main.cpp`**：会得到 `undefined reference to sum/average`；同一个 target 要包含 `link_stats.cpp`。

> **记忆点：** CMake 最常见的两个坑：多个 `main` 塞一个 target，以及把故意失败的文件加进构建。

## 8. 自测

- `add_executable`、`target_compile_features`、`target_compile_options` 各自做什么？
- 为什么 `d03-linkage` 可以有两个 `.cpp`，而 `d03-overload` 和 `d03-debug` 不能合在一起？
- `-S` 和 `-B` 分别是什么？
- `cmake --build --target d03-debug` 和 `cmake --build` 有什么区别？
- 为什么 `fail_*.cpp` 不能加进 CMake？
- 怎么确认 `d03-debug` 真的带了 `-g -O0`？
- 为什么改了 `CMakeLists.txt` 后要重新配置？
- `d03-debug` 当前输出为什么是 `10/2`？怎么改成 `15/3`？
