# D02 任务 6：接入 CMake

> 环境：WSL2 Ubuntu 24.04，CMake 3.28.3，Ninja 1.11.1，g++ 13.3.0。
> 源码目录：`/mnt/d/Project/CPP/CPPLearn/base`。
> 构建目录：`~/build/base`（out-of-source，源码树外构建）。
>
> Git 部分已经由你完成；这篇笔记只记录 CMake 接入，不重复执行 Git 操作。

## 0. 速查

```bash
# 配置：读 CMakeLists.txt，生成 Ninja 构建文件
cmake -S /mnt/d/Project/CPP/CPPLearn/base -B ~/build/base -G Ninja

# 构建全部 target
cmake --build ~/build/base -j"$(nproc)"

# 只构建一个 target
cmake --build ~/build/base --target d02-types -j"$(nproc)"
cmake --build ~/build/base --target d02-pointers -j"$(nproc)"

# 运行
~/build/base/d02-types
~/build/base/d02-pointers

# 清掉构建产物，保留配置缓存
cmake --build ~/build/base --target clean

# 彻底重来：连缓存和生成文件一起删
rm -rf ~/build/base
cmake -S /mnt/d/Project/CPP/CPPLearn/base -B ~/build/base -G Ninja
```

## 6.1 CMake 在构建流程里的位置

CMake **不是编译器**，它只负责“生成构建文件”：

```text
CMakeLists.txt
      │  cmake -S ... -B ...
      ▼
CMakeCache.txt + build.ninja
      │  cmake --build ...（实际调用 ninja）
      ▼
g++ 编译 / ld 链接
      ▼
~/build/base/d02-types、d02-pointers
```

三个阶段要分清：

1. **configure（配置）**：读 `CMakeLists.txt`，找编译器，检查环境，把变量写进 `CMakeCache.txt`。
2. **generate（生成）**：根据配置写出 `build.ninja`（或 Makefile、Visual Studio 工程）。
3. **build（构建）**：Ninja 按依赖图调用 `g++` 和链接器，生成可执行文件。

`cmake --build` 既不是“再配置一次”，也不是“直接调 g++”；它是调用底层生成器（这里是 Ninja）去执行构建。Ninja 的构建文件不要手改，改 `CMakeLists.txt` 再重新生成。

CMake 的核心单位是 **target（目标）**：一个可执行文件、一个库，或一个自定义目标。本任务的两个 target 是 `d02-types` 和 `d02-pointers`，它们互相独立，可以单独构建。

### 为什么 out-of-source

- 源码树里不会出现 `.o`、可执行文件、`CMakeCache.txt`，`git status` 保持干净。
- 可以同时存在多个构建目录：`~/build/base-debug`、`~/build/base-release`，共用一份源码。
- 想彻底重置时，删构建目录即可，不会碰源码。
- 本任务把构建目录放在 WSL 的 `~/build/base`，而不是 `/mnt/d`：WSL 访问 Windows 盘走 9p 文件系统，编译大量小文件会明显更慢。

## 6.2 修改后的 `base/CMakeLists.txt`

当前完整内容：

```cmake
cmake_minimum_required(VERSION 3.28)
project(CPPLearn LANGUAGES CXX)

# 用 -std=c++20，而不是 -std=gnu++20。
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(d01-hello days/d01-toolchain/hello.cpp)

# D02：每个练习文件一个独立可执行 target。
add_executable(d02-types days/d02-basics/code/types.cpp)
target_compile_features(d02-types PRIVATE cxx_std_20)
target_compile_options(d02-types PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror)

add_executable(d02-pointers days/d02-basics/code/pointers.cpp)
target_compile_features(d02-pointers PRIVATE cxx_std_20)
target_compile_options(d02-pointers PRIVATE
    -Wall -Wextra -Wpedantic -Wconversion -Werror)
```

逐项解释：

| 语句 | 作用 |
|---|---|
| `cmake_minimum_required(VERSION 3.28)` | 要求 CMake 至少 3.28，同时设定策略（policy）默认值；低于这个版本会直接报错 |
| `project(CPPLearn LANGUAGES CXX)` | 定义项目名并启用 C++；CMake 从这里开始检测 C++ 编译器 |
| `set(CMAKE_CXX_EXTENSIONS OFF)` | 关闭 GNU 扩展，让 CMake 生成 `-std=c++20` 而不是 `-std=gnu++20` |
| `add_executable(d02-types ...)` | 创建一个名为 `d02-types` 的可执行 target；target 名默认也是输出文件名 |
| `target_compile_features(... cxx_std_20)` | 声明这个 target 需要 C++20；CMake 负责加对应的 `-std=` 选项 |
| `target_compile_options(... PRIVATE ...)` | 只给这个 target 加编译选项；`PRIVATE` 表示不向依赖它的 target 传播 |
| `-Wall -Wextra -Wpedantic` | 打开常用警告，`-Wpedantic` 检查标准之外的写法 |
| `-Wconversion` | 检查可能改变值的隐式转换（任务 1 的重点） |
| `-Werror` | 把警告升级为错误；本仓库用它把“零警告”变成硬性门槛 |

几个容易误解的点：

- **`target_compile_features` 不是“只检查编译器支持”**：它还会让 CMake 选择对应的标准开关。`cxx_std_20` 在这里最终变成 `-std=c++20`。
- **`CMAKE_CXX_EXTENSIONS OFF` 是全局设置**：它影响之后创建的所有 target。`d01-hello` 没有声明标准，实际得到的是 `-std=c++17`（编译器默认标准的 ISO 版本）；两个 D02 target 因为声明了 `cxx_std_20`，得到 `-std=c++20`。
- **`-Werror` 只适合学习/CI 的严格检查**：真实项目里，换一个编译器版本就可能多出新警告，所以很多团队不在本地默认开 `-Werror`，而是在 CI 里开。
- 这些警告选项是 GCC/Clang 写法；MSVC 对应的是 `/W4 /WX`。本项目在 WSL + g++ 下构建，所以直接写。

## 6.3 配置：第一次和已有缓存

### 第一次配置（空构建目录）

另开一个空目录 `/tmp/d02_cmake_fresh` 演示首次配置的实际输出：

```bash
cmake -S /mnt/d/Project/CPP/CPPLearn/base -B /tmp/d02_cmake_fresh -G Ninja
```

```text
-- The CXX compiler identification is GNU 13.3.0
-- Detecting CXX compiler ABI info
-- Detecting CXX compiler ABI info - done
-- Check for working CXX compiler: /usr/bin/c++ - skipped
-- Detecting CXX compile features
-- Detecting CXX compile features - done
-- Configuring done (0.7s)
-- Generating done (0.0s)
-- Build files have been written to: /tmp/d02_cmake_fresh
```

第一次会做编译器检测；这些结果会缓存到 `CMakeCache.txt`，以后不会每次重复。

### 已有缓存时重新配置

本任务的 `~/build/base` 之前已经配置过，所以输出只有：

```bash
cmake -S /mnt/d/Project/CPP/CPPLearn/base -B ~/build/base -G Ninja
```

```text
-- Configuring done (0.0s)
-- Generating done (0.0s)
-- Build files have been written to: /home/chengyuchao/build/base
```

`-G Ninja` 指定生成器。已有缓存已经是 Ninja，重复指定相同生成器没问题；如果缓存用的是别的生成器，CMake 会报错，要求你换目录或删掉旧缓存。

缓存里能看到这三项关键信息：

```text
CMAKE_GENERATOR:INTERNAL=Ninja
CMAKE_CXX_COMPILER:FILEPATH=/usr/bin/c++
CMAKE_HOME_DIRECTORY:INTERNAL=/mnt/d/Project/CPP/CPPLearn/base
```

`CMAKE_HOME_DIRECTORY` 记录这份缓存对应的源码目录。**源码目录移动过，旧构建目录就不能直接用了**，要么重新配置到新目录，要么删掉重建。

## 6.4 构建：两个 target、零警告

从干净状态构建两个 D02 target：

```bash
cmake --build ~/build/base --clean-first \
    --target d02-types d02-pointers -j"$(nproc)"
```

实际输出：

```text
[1/1] Cleaning all built files...
Cleaning... 4 files.
[1/4] Building CXX object CMakeFiles/d02-types.dir/days/d02-basics/code/types.cpp.o
[2/4] Building CXX object CMakeFiles/d02-pointers.dir/days/d02-basics/code/pointers.cpp.o
[3/4] Linking CXX executable d02-types
[4/4] Linking CXX executable d02-pointers
```

- 没有任何 `warning:` 行；`-Werror` 又保证了“有警告就失败”。
- `--clean-first` 会先清理**整个构建树**，再构建指定 target；不是只清理这两个 target。清理后 `d01-hello` 也会被删掉，后面执行一次全量构建才恢复。
- `-j"$(nproc)"` 用满 CPU 核数并行编译。小项目看不出差别，项目大了很明显。

用 Ninja 的 `-t commands` 可以直接看到 CMake 最终生成的编译命令：

```bash
ninja -C ~/build/base -t commands d02-types | head -n 1
```

```text
/usr/bin/c++   -std=c++20 -Wall -Wextra -Wpedantic -Wconversion -Werror -MD -MT CMakeFiles/d02-types.dir/days/d02-basics/code/types.cpp.o -MF CMakeFiles/d02-types.dir/days/d02-basics/code/types.cpp.o.d -o CMakeFiles/d02-types.dir/days/d02-basics/code/types.cpp.o -c /mnt/d/Project/CPP/CPPLearn/base/days/d02-basics/code/types.cpp
```

- `-std=c++20` 来自 `target_compile_features(... cxx_std_20)` + `CMAKE_CXX_EXTENSIONS OFF`。
- `-Wall -Wextra -Wpedantic -Wconversion -Werror` 来自 `target_compile_options`。
- `-MD -MT -MF` 是 CMake/Ninja 用来生成头文件依赖文件的，不是我们手写的选项。
- 编译命令里的源文件路径是绝对路径，但 `CMakeLists.txt` 里写的是相对 `base/` 的路径。

全量构建（包括 `d01-hello`）：

```bash
cmake --build ~/build/base -j"$(nproc)"
```

```text
[1/2] Building CXX object CMakeFiles/d01-hello.dir/days/d01-toolchain/hello.cpp.o
[2/2] Linking CXX executable d01-hello
```

## 6.5 运行与验证

```bash
~/build/base/d02-types
~/build/base/d02-pointers
```

两个程序的退出码都是 `0`：

```text
$ ~/build/base/d02-types > /dev/null; echo $?
0
$ ~/build/base/d02-pointers > /dev/null; echo $?
0
```

`d02-types` 的输出开头：

```text
== 1. 固定宽度整数类型 ==
int8_t      sizeof = 1   min = -128, max = 127
int16_t     sizeof = 2   min = -32768, max = 32767
int32_t     sizeof = 4   min = -2147483648, max = 2147483647
int64_t     sizeof = 8   min = -9223372036854775808, max = 9223372036854775807
...
```

结尾：

```text
== 8. constexpr ==
with_cout(4) called
with_cout(3) called
square(6) = 36, square(7) = 49
with_cout(3) = 9, runtime_const = 16
== 9. auto ==
a = 42, r = 1, r2 = 1, y = 42, *p = 1, sizeof(arr_ref) = 20
```

`d02-pointers` 的第 5、6 节：

```text
== 5. const 正确性 ==
main before:  data = 0x625de869d2f0, data[0] = 10
process_ref:  data = 0x625de869d2f0, v[0] = 10
process_val:  data = 0x625de869d310, v[0] = -1
main after:   data = 0x625de869d2f0, data[0] = 10
== 6. 数组退化 ==
main 里 sizeof(arr)  = 20
函数内 sizeof(arr)   = 8（arr 已退化为指针）
引用数组 sizeof(arr) = 20（引用到数组，未退化）
```

地址每次运行会变；关键结论（按值传参会拷贝、数组退化为指针）不变。完整分析见 [note_task3.md](note_task3.md)。

## 6.6 构建目录里有什么

```text
~/build/base/
├── CMakeCache.txt        # 配置缓存：编译器、生成器、源码路径、变量
├── build.ninja           # CMake 生成的构建规则，不要手改
├── CMakeFiles/           # 每个 target 的 .o、依赖文件、内部规则
│   ├── d02-types.dir/days/d02-basics/code/types.cpp.o
│   └── d02-pointers.dir/days/d02-basics/code/pointers.cpp.o
├── cmake_install.cmake
├── d01-hello
├── d02-types
└── d02-pointers
```

实际 `ls -la ~/build/base`：

```text
total 124
drwxr-xr-x 3 chengyuchao chengyuchao  4096 Nov  5 22:56 .
drwxr-xr-x 3 chengyuchao chengyuchao  4096 Oct 27 23:45 ..
-rw-r--r-- 1 chengyuchao chengyuchao 14964 Nov  5 22:56 .ninja_deps
-rw-r--r-- 1 chengyuchao chengyuchao   686 Nov  5 22:56 .ninja_log
-rw-r--r-- 1 chengyuchao chengyuchao 12376 Oct 27 23:45 CMakeCache.txt
drwxr-xr-x 7 chengyuchao chengyuchao  4096 Nov  5 22:56 CMakeFiles
-rw-r--r-- 1 chengyuchao chengyuchao  8777 Nov  5 22:56 build.ninja
-rw-r--r-- 1 chengyuchao chengyuchao  1630 Oct 27 23:45 cmake_install.cmake
-rwxr-xr-x 1 chengyuchao chengyuchao 36200 Nov  5 22:56 d02-pointers
-rwxr-xr-x 1 chengyuchao chengyuchao 24000 Nov  5 22:56 d02-types
```

几个观察：

- 目标文件路径保留了源码的相对目录结构：`CMakeFiles/d02-types.dir/days/d02-basics/code/types.cpp.o`。
- 可执行文件默认放在构建目录根下，因为没有设置 `RUNTIME_OUTPUT_DIRECTORY`。多配置生成器（Visual Studio、Xcode）会再套一层 `Debug/` 或 `Release/`。
- `CMakeCache.txt` 是缓存，不是源码；不要手改，也不要提交。想回到“完全没配置过”的状态，删整个构建目录最干净。
- 仓库里如果还看到 `base/build/base`，那是另一份构建目录；本任务统一用 `~/build/base`，不要把两边的缓存混用。

## 6.7 常用命令与参数

| 命令 / 参数 | 作用 |
|---|---|
| `cmake -S <src> -B <build>` | 指定源码目录和构建目录，配置项目 |
| `cmake -S <src> -B <build> -G Ninja` | 指定 Ninja 生成器；空目录首次配置时用 |
| `cmake -S <src> -B <build> -DCMAKE_BUILD_TYPE=Debug` | 单配置生成器（Ninja/Make）设置构建类型 |
| `cmake --build <build>` | 构建默认 `all` 目标 |
| `cmake --build <build> --target d02-types` | 只构建指定 target |
| `cmake --build <build> -j N` | 并行构建，`-j"$(nproc)"` 用满核 |
| `cmake --build <build> --clean-first` | 先清理整个构建树再构建 |
| `cmake --build <build> --verbose` | 打印实际执行的编译/链接命令 |
| `cmake --build <build> --target clean` | 只清理构建产物，保留缓存 |
| `cmake --install <build>` | 安装（本项目暂时用不到） |
| `cmake --version` | 查看 CMake 版本，确认满足 `cmake_minimum_required` |

查看实际编译命令还有两种方式：

```bash
cmake --build ~/build/base --target d02-types --verbose
ninja -C ~/build/base -t commands d02-types
```

`CMAKE_BUILD_TYPE` 在当前缓存里是空的：

```text
CMAKE_BUILD_TYPE:STRING=
```

这意味着 CMake 没有额外加 `-O2`、`-g` 之类的优化/调试选项。日常调试建议重新配置：

```bash
cmake -S /mnt/d/Project/CPP/CPPLearn/base -B ~/build/base \
    -DCMAKE_BUILD_TYPE=Debug
```

单配置生成器要在 **configure 阶段**设置；已经生成后再改缓存变量，需要重新运行 `cmake`。

## 6.8 增量构建与重新配置

没有改动时再构建：

```bash
cmake --build ~/build/base --target d02-types d02-pointers -j2
```

```text
ninja: no work to do.
```

只改 `types.cpp` 时，Ninja 只会重编 `types.cpp.o` 并重新链接 `d02-types`，不会动 `d02-pointers`；头文件依赖由 `-MD` 生成的 `.d` 文件跟踪。

改 `CMakeLists.txt` 后，`cmake --build` 会自动重新运行 CMake（Ninja 里有 regeneration 规则）；也可以手动再执行一次 `cmake -S ... -B ...`。但如果改的是编译器、生成器这类根配置，最稳妥的做法还是删掉构建目录重来。

`--clean-first` 和 `--target clean` 的区别：

- `--target clean`：只清理，不构建。
- `--clean-first`：先清理整个构建树，再构建指定目标。
- `rm -rf ~/build/base`：连缓存、生成器状态一起删，下一次配置会重新检测编译器。

## 6.9 为什么 ASan/UBSan 不放进 CMake

任务要求里说得很清楚：需要 UBSan/ASan 版本时，**临时用命令行编译**，不改 CMake；D7 再工程化。这样做的好处是 CMake 保持简单，不让每个 target 都背上一套调试配置。

UBSan（`types.cpp` 的有符号溢出演示）：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion \
    -DUB_DEMO -fsanitize=undefined -fno-sanitize-recover=all -g \
    base/days/d02-basics/code/types.cpp -o /tmp/types_ubsan
```

ASan（`pointers.cpp` 的悬空指针演示）：

```bash
g++ -std=c++20 -Wall -Wextra -Wpedantic -Wconversion \
    -DASAN_DEMO -Wno-return-local-addr -fsanitize=address -g \
    base/days/d02-basics/code/pointers.cpp -o /tmp/pointers_asan
```

`-Wno-return-local-addr` 只是为了放行“故意返回局部变量地址”的演示；正常代码不要用它掩盖问题。实际报错见 [note_task1.md](note_task1.md) 和 [note_task3.md](note_task3.md)。

如果以后要把 sanitizer 工程化，CMake 里至少要同时加**编译选项**和**链接选项**：

```cmake
# 以后再做，不是本任务的改动
target_compile_options(d02-pointers PRIVATE -fsanitize=address -g)
target_link_options(d02-pointers PRIVATE -fsanitize=address)
```

只加 `target_compile_options` 不加 `target_link_options`，编译能过，但链接时会缺 sanitizer runtime 符号。这也是 D7 要单独处理构建配置的原因。

## 6.10 易错点

1. **把 configure 当成 build**：`cmake -S ... -B ...` 只生成构建文件，不产出可执行文件；必须再 `cmake --build`。
2. **把 target 名写成源文件名**：`--target d02-types` 用的是 `add_executable` 的第一个参数，不是 `types.cpp`。
3. **在源码目录里直接 `cmake .`**：会生成 in-source 的 `CMakeCache.txt`、`CMakeFiles/`，污染仓库；始终用 `-S`/`-B`。
4. **混用两个构建目录**：`~/build/base` 和仓库里的 `base/build/base` 各有自己的缓存；在 A 目录构建却去 B 目录找产物，会以为“没生成”。
5. **移动源码后还用旧缓存**：`CMAKE_HOME_DIRECTORY` 对不上，配置会失败或行为异常；删构建目录重配。
6. **改 `CMakeLists.txt` 后不重新配置**：正常 `cmake --build` 会自动重新生成；如果用 `ninja` 直接构建且自动规则被破坏，要手动跑一次 `cmake`。
7. **以为 `-j` 是 CMake 的参数**：并行是 `cmake --build ... -j N`，不是 `cmake -j N -S ... -B ...`。
8. **`-Werror` 与编译器差异**：GCC 能过的警告集合，Clang/MSVC 不一定一样；跨平台项目要做条件判断，不能盲目复制。
9. **只加编译选项，忘了链接选项**：`-fsanitize=...`、`-pthread` 这类选项经常编译和链接都要；分别用 `target_compile_options` 和 `target_link_options`。
10. **`--clean-first` 不是只清一个 target**：它清整个构建树；只想清某个 target 的产物，用 `--target clean` 或手动删对应文件。
11. **删构建目录前不确认路径**：`rm -rf ~/build/base` 是对的；不要写成变量为空时变成 `rm -rf /` 之类的命令。
12. **把生成文件提交进 Git**：`CMakeCache.txt`、`build.ninja`、`.o`、可执行文件都不应提交；构建目录放在源码树外最省心。

## 6.11 待复习

- CMake、Ninja、g++ 三者各自负责什么？`cmake --build` 实际调用了谁？
- configure / generate / build 三个阶段分别产出什么？
- `add_executable` 的第一个参数是什么？`--target` 用的是哪个名字？
- `target_compile_features(... cxx_std_20)` 最终变成了哪个编译选项？
- `CMAKE_CXX_EXTENSIONS OFF` 解决了什么问题？`-std=c++20` 和 `-std=gnu++20` 差在哪？
- `PRIVATE` 是什么意思？可执行 target 和库 target 在这里有区别吗？
- 为什么 `-Wconversion` 要配合 `-Werror` 才能当“零警告”的门槛？
- `--clean-first`、`--target clean`、`rm -rf build` 三者的区别？
- 怎么只构建一个 target？怎么查看实际编译命令？
- `CMAKE_BUILD_TYPE` 在 Ninja 这种单配置生成器里什么时候设置？
- 为什么 sanitizer 的 `-fsanitize=address` 编译和链接都要加？
- 构建目录为什么放在 `~/build/base` 而不是 `/mnt/d/.../base/build`？

## 6.12 Git 部分

Git 已由你完成，本笔记不重复执行任何 Git 命令，也不改动提交历史。

只记一条原则：**构建目录不进入版本库**。`~/build/base` 本来就在源码树外；如果以后在仓库内建构建目录，要确保它被 `.gitignore` 忽略。
