# C++ 核心知识两周学习计划

> 周期：14 天（建议每天 4–5 小时有效学习）
> 目标岗位方向：C++ / Linux 后台开发
> 所有代码、笔记、文档均放在本目录（`base/`）下。

## 1. 学习目标

两周结束时，应具备以下能力：

- 掌握 C++17/20 的核心语言特性与标准库用法，能写出现代 C++ 风格代码；
- 在 Linux 环境下使用 CMake 构建项目、GDB 调试程序；
- 理解对象生命周期、RAII、所有权模型与内存管理；
- 理解线程同步机制，能写出无线程竞争、无死锁的并发程序；
- 能使用 POSIX Socket 与 epoll 编写基础的 TCP 服务器；
- 会使用 Git 管理代码，能用性能分析工具定位问题。

### 本次不深入的内容

协程、Modules、深度模板元编程、自定义分配器实现、网络框架源码（muduo/libuv）、分布式与协议栈实现。这些留作两周后的专项进阶。

## 2. 学习环境

| 项目 | 选择 |
|---|---|
| 操作系统 | VirtualBox + Ubuntu（已安装；Linux 是目标技能，直接作为开发环境） |
| 编译器 | GCC 13+，统一使用 `-std=c++20` |
| 构建 | CMake 3.28+ + Ninja |
| 调试 | GDB |
| 版本管理 | Git |
| 内存/性能工具 | valgrind、ASan/UBSan、TSan、Google Benchmark、perf（见备注） |
| 编辑器 | Windows 端 VS Code + Remote-SSH + C/C++ + CMake Tools |

**统一编译选项**

```bash
-Wall -Wextra -Wpedantic -Wconversion -std=c++20
# 调试构建附加：-g -O0
# 内存检查附加：-fsanitize=address,undefined -fno-omit-frame-pointer
# 并发检查附加：-fsanitize=thread
```

### 开发环境接入（VirtualBox）

1. **共享文件夹（永久）**：在 VirtualBox 中将 `D:\Project\CPP\CPPLearn` 注册为共享文件夹 `CPPLearn`，Ubuntu 的 `/etc/fstab` 加入：

   ```
   CPPLearn  /media/sf_CPPLearn  vboxsf  defaults,nofail  0  0
   ```

   然后执行 `sudo mkdir -p /media/sf_CPPLearn && sudo mount -a`，并建立软链接：

   ```bash
   ln -s /media/sf_CPPLearn ~/CPPLearn
   ```

2. **静态 IP + SSH**：网卡 1 用 NAT 上网，网卡 2 用 Host-Only；Ubuntu 内配置静态 IP `192.168.56.101/24`，安装 `openssh-server` 并配置密钥登录；Windows 端 VS Code Remote-SSH 连接 `chengyc@192.168.56.101`，打开 `~/CPPLearn/base` 编辑、编译、调试。

3. **构建目录不要放在共享文件夹**（vboxsf 小文件读写较慢）。源码放共享目录，构建产物放虚拟机原生磁盘：

   ```bash
   cmake -S ~/CPPLearn/base -B ~/build/base
   cmake --build ~/build/base
   ```

**备注**：VirtualBox 虚拟机通常也拿不到硬件 PMU，`perf stat` 的硬件事件可能不可用。学习阶段优先使用 valgrind/callgrind、gprof、Google Benchmark；真实 `perf` 练习建议放在物理 Linux 机器或云主机上。

## 3. 总体安排

| 阶段 | 时间 | 主题 |
|---|---|---|
| 第一阶段 | Day 1–7 | C++ 语言核心 + Linux/Git/GDB/CMake 工具链 |
| 第二阶段 | Day 8–14 | STL、内存管理、异常与移动语义、并发、网络、性能分析 |

**每天固定节奏**（约 4–5 小时）：

| 环节 | 时长 | 做法 |
|---|---|---|
| 概念学习 | 1h | 看 cppreference / 教程，只学当天范围 |
| 动手编码 | 2.5h | 先复现示例，再独立完成当日产出 |
| 笔记复盘 | 1h | 写 `note.md`，记录易错点并提交 Git |

## 4. Week 1：语言核心与工程工具

| 天 | 主题 | 关键内容 | 产出 |
|---|---|---|---|
| D1 | 环境 + 编译模型 + Git | Ubuntu 内检查/安装 GCC、CMake、GDB、Git；共享目录与 SSH 接入；预处理→编译→汇编→链接；头文件与 ODR；`git init/add/commit/log/diff` | 首个 CMake 工程；仓库初始化；`docs/toolchain.md` |
| D2 | 基础类型 + Linux 基础 | 基本类型与初始化；`const/constexpr/auto`；引用与指针；左值/右值初识；shell、文件权限、进程、信号、`strace` 入门 | 类型与指针练习集；`docs/linux-basics.md` |
| D3 | 函数、链接、GDB 入门 | 重载解析、默认参数、命名空间、`static`/匿名命名空间；编译错误 vs 链接错误；`break/bt/frame/watch/p`；Git 分支与合并 | 多文件工程，用 GDB 完成一次断点调试 |
| D4 | 类与对象 I：生命周期 | 构造/析构、初始化列表、`explicit`、拷贝构造、Rule of 0/3/5、RAII | 手写 `String` 类 + `FileHandle` RAII 包装 |
| D5 | 类与对象 II：继承与多态 | 虚函数、`override/final`、虚析构、纯虚类；vtable 机制；对象切片；`dynamic_cast`；用 GDB 观察虚调用 | `Shape` 继承体系 + 多态容器 |
| D6 | 运算符重载 + C++11/14/17 | 成员 vs 非成员运算符、`operator<<`、`<=>`、类型转换；lambda、`auto`、移动语义（11）；泛型 lambda、`make_unique`（14）；structured bindings、`if constexpr`、`optional/variant/string_view/filesystem`、fold expressions、`[[nodiscard]]`（17） | `Vec3` 完整实现 + 特性示例集 |
| D7 | 模板入门 + C++20 + CMake 工程化 | 函数/类模板、推导、特化、可变参数模板；concepts、ranges、`<=>`、designated initializers、`consteval/constinit`、`span`、`format`；`add_library`、include 路径、Ctest、Presets、sanitizer 选项 | **项目 A**：核心综合（见第 7 节） |

## 5. Week 2：标准库、系统、并发与网络

| 天 | 主题 | 关键内容 | 产出 |
|---|---|---|---|
| D8 | STL 容器与算法 | 容器选型与复杂度、迭代器失效规则、`reserve/emplace`；迭代器分类、`<algorithm>`、lambda、`<ranges>` pipeline | 词频统计 + 算法练习集 |
| D9 | 内存管理专题 | 栈/堆/静态区布局、对齐、`placement new`、allocator 概览、缓存局部性；`unique_ptr/shared_ptr/weak_ptr`、循环引用；ASan/valgrind 检测泄漏与越界 | 内存问题复现集 + 修复版 |
| D10 | 异常 + 移动语义 | 栈展开、`noexcept`、异常安全等级、构造/析构中抛异常；`optional` 错误处理；`move/forward`、引用折叠、完美转发 | 文件解析器：错误处理重构 |
| D11 | 并发 I：线程与同步 | `thread/jthread`；mutex 家族、`lock_guard/unique_lock/scoped_lock`；条件变量与虚假唤醒；`atomic` 与内存序（relaxed/acquire/release/seq_cst）；数据竞争与死锁 | 生产者–消费者队列；多线程 GDB 调试记录 |
| D12 | 并发 II：线程池与工具 | 固定大小线程池实现；`semaphore/latch/barrier`；false sharing；TSan 检测数据竞争 | 线程池 + TSan 验证报告 |
| D13 | 网络通信 I | POSIX socket API、TCP/UDP 流程、字节序、非阻塞 IO、`epoll`、`SO_REUSEADDR`、SIGPIPE、粘包/拆包 | 单线程 epoll TCP echo server |
| D14 | 网络 II + 性能分析 | 线程池 + epoll 组合服务器；`perf record/report`、火焰图、`strace`、callgrind、Google Benchmark | **项目 B**：多线程 TCP 服务器 + 优化报告 |

## 6. 技能覆盖对照

| 能力 | 覆盖位置 | 预期深度 |
|---|---|---|
| C++17/20 | D6、D7，其余每天使用 | 熟练使用核心特性 |
| Linux | D2 基础，D11–D14 系统编程 | 能日常开发、调试、用系统工具 |
| CMake | D1 入门、D7 工程化，项目全程使用 | 能组织多目标工程 |
| GDB | D3 入门、D11 多线程、D9 内存问题 | 能独立定位常见问题 |
| Git | D1 初始化、D3 分支，每天提交 | 日常开发流程 |
| 线程同步 | D11、D12 | 理解并能正确实现 |
| 内存管理 | D4、D9、D10 | 理解生命周期与所有权，会用工具检测 |
| 网络通信 | D13、D14 | 入门：能写基础 TCP 服务器 |
| 性能分析 | D14 + 项目 B | 入门：会用工具、能读报告 |

> 说明：网络与性能分析在两周内只能达到入门水平，后续建议各安排 1–2 周专项深入。

## 7. 综合项目

### 项目 A：C++ 核心综合（Day 7）

- 手写 `String` 类，满足 Rule of 5；
- `Shape` 抽象基类 + 多个派生类，使用多态容器；
- 封装为静态库，编写单元测试（Catch2 或 GoogleTest）；
- 使用 CMake 组织构建并接入 Ctest。

### 项目 B：多线程 TCP 服务器（Day 11–14）

- RAII 封装 socket 与文件描述符；
- epoll 事件循环 + 固定大小线程池；
- 支持多客户端连接与简单协议帧（长度前缀，处理粘包）；
- 记录日志，使用 TSan/ASan 验证；
- 用 Google Benchmark 或压测脚本给出优化前后对比报告。

## 8. 目录结构

```
base/
├── README.md                 # 本文件：总规划与打卡表
├── docs/
│   ├── toolchain.md          # 环境安装、编译/调试命令
│   ├── linux-basics.md       # Linux 常用命令与系统概念
│   ├── cheatsheet.md         # 语法/STL 速查（随进度补）
│   └── pitfalls.md           # 易错点与未定义行为记录
├── days/
│   ├── d01-toolchain/
│   │   ├── note.md
│   │   └── *.cpp
│   ├── d02-basics/
│   └── ...
├── projects/
│   ├── a-cpp-core/           # 项目 A
│   └── b-tcp-server/         # 项目 B
└── CMakeLists.txt            # 顶层构建（项目 A/B 接入）
```

**约定**

- 每天的代码放在 `days/dNN-<主题>/`，每个 `.cpp` 尽量独立可编译；
- 每天至少一次 Git 提交，提交信息格式：`dNN: 主题`；
- 遇到未定义行为或踩坑，立刻记入 `docs/pitfalls.md`；
- 所有示例统一使用 C++20，不使用编译器私有扩展。

## 9. 每日笔记模板

```markdown
# DNN - 主题

## 今天解决的问题
## 核心概念 & 为什么这样设计
## 易错点 / UB（附最小复现代码）
## 代码片段
## 待复习
```

## 10. 验收标准

- [ ] 能解释对象生命周期、拷贝 vs 移动、虚函数调用过程、RAII、所有权模型；
- [ ] 不看资料手写符合 Rule of 5 的类，且 ASan 下无内存错误；
- [ ] 熟练使用常用 STL 容器与算法，能说出迭代器失效条件；
- [ ] 能写出无数据竞争的生产者–消费者与线程池，并通过 TSan；
- [ ] 能用 GDB 定位崩溃、死循环、多线程问题；
- [ ] 能独立用 CMake 组织多目标工程并接入 Ctest；
- [ ] 项目 B 可编译、可运行、有测试、有性能对比报告。

## 11. 进度打卡

**开始日期：____ 年 __ 月 __ 日**

### Week 1

- [ ] D1 环境 + 编译模型 + Git
- [ ] D2 基础类型 + Linux 基础
- [ ] D3 函数、链接、GDB 入门
- [ ] D4 类与对象 I：生命周期
- [ ] D5 类与对象 II：继承与多态
- [ ] D6 运算符重载 + C++11/14/17
- [ ] D7 模板 + C++20 + CMake 工程化 + 项目 A

### Week 2

- [ ] D8 STL 容器与算法
- [ ] D9 内存管理专题
- [ ] D10 异常 + 移动语义
- [ ] D11 并发 I：线程与同步
- [ ] D12 并发 II：线程池与工具
- [ ] D13 网络通信 I：socket 与 epoll
- [ ] D14 网络 II + 性能分析 + 项目 B

## 12. 参考资源

| 用途 | 资源 |
|---|---|
| 语言参考 | cppreference.com（首选，遇到问题先查） |
| 系统教程 | learncpp.com |
| 查漏补缺 | 《C++ Primer》第 5 版 |
| 现代 C++ 实践 | 《Effective Modern C++》 |
| 汇编/模板观察 | Compiler Explorer（godbolt.org）、C++ Insights |
| 网络编程 | 《UNIX 网络编程》卷 1（按需查阅） |
| Linux 系统编程 | 《Linux/UNIX 系统编程手册》或 man 手册 |
| 并发 | 《C++ Concurrency in Action》第 2 版（选读前 5 章） |
