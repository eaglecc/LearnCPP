# D02 任务 5：Linux 基础

> 环境：WSL2 Ubuntu 24.04，x86_64，普通用户 `uid=1000(chengyuchao)`，`umask=0022`。
>
> 实验目录：`/tmp/d02_linux_lab`。所有实验都在这个临时目录里做，避免改动仓库文件的权限。
> 下面的 PID、地址、时间、磁盘数字每次运行都会变；命令和字段含义不变。
>
> 任务清单里的产物名是 `docs/linux-basics.md`；这里按 d02 的笔记序列整理成 `note_task5.md`。

## 0. 速查

- **权限**：`r=4`、`w=2`、`x=1`；`rwxr-xr-x = 755`，`rw-r--r-- = 644`，`rw------- = 600`。
- **目录的 `x`**：能进入/遍历目录（`cd`、访问 `dir/file`）；**目录的 `r`**：能列出目录项名字。
- **重定向**：`>` 覆盖 stdout，`>>` 追加 stdout，`2>` 重定向 stderr，`&>` 同时重定向 stdout+stderr，`|` 把左边 stdout 接到右边 stdin。
- **信号**：`SIGINT=2`（Ctrl+C）、`SIGTERM=15`（可捕获的终止）、`SIGKILL=9`（不可捕获）、`SIGHUP=1`、`SIGPIPE=13`。
- **`SIGTERM` vs `SIGKILL`**：前者可以被捕获/忽略/阻塞，允许清理；后者由内核强制终止，不能捕获/忽略/阻塞。
- **`strace`**：`-e trace=openat` 看打开了哪些文件；`-c` 看 syscall 统计；`write(1, ...)` 的 `1` 是 stdout 的文件描述符。

## 5.1 Shell 与文件

### 5.1.1 准备实验目录

```bash
LAB=/tmp/d02_linux_lab
rm -rf "$LAB"
mkdir -p "$LAB/subdir"
cd "$LAB"

printf 'alpha\nTODO: fix me\n' > file.txt
printf 'beta\n' > subdir/other.cpp
printf 'TODO: second\n' > subdir/todo.cpp
: > empty.txt
```

`ls -la` 实际输出：

```text
total 16
drwxr-xr-x  3 chengyuchao chengyuchao 4096 Nov  4 23:35 .
drwxrwxrwt 10 root        root        4096 Nov  4 23:35 ..
-rw-r--r--  1 chengyuchao chengyuchao    0 Nov  4 23:35 empty.txt
-rw-r--r--  1 chengyuchao chengyuchao   19 Nov  4 23:35 file.txt
drwxr-xr-x  2 chengyuchao chengyuchao 4096 Nov  4 23:35 subdir
```

- `-l` 长格式，`-a` 包含 `.`、`..` 和隐藏文件。
- 第一列是类型+权限：`-` 普通文件，`d` 目录；后面 9 位是 `owner/group/other` 的 `rwx`。
- 第二列硬链接数；第三、四列 owner/group；第五列字节数（目录通常是 4096）；后面是时间、名字。

### 5.1.2 `stat`：看 inode 和权限

```bash
stat file.txt
```

实际输出：

```text
  File: file.txt
  Size: 19        	Blocks: 8          IO Block: 4096   regular file
Device: 8,48	Inode: 64267       Links: 1
Access: (0644/-rw-r--r--)  Uid: ( 1000/chengyuchao)   Gid: ( 1000/chengyuchao)
Access: 2026-11-04 23:35:28.708318601 +0800
Modify: 2026-11-04 23:35:28.708318601 +0800
Change: 2026-11-04 23:35:28.708318601 +0800
 Birth: 2026-11-04 23:35:28.708318601 +0800
```

- `Size`：文件内容字节数；`Blocks`：实际占用的 512B 块数；`IO Block`：文件系统 I/O 块大小。
- `Device`：设备号；`Inode`：inode 编号；`Links`：硬链接数。
- `Access` 第一行是权限；后面三个时间分别是 atime（访问）、mtime（内容修改）、ctime（inode 状态修改，不是创建时间）；`Birth` 是创建时间（文件系统支持时）。
- 只看权限可以用更紧凑的格式：

```bash
stat -c '%a %A %n' file.txt
```

### 5.1.3 权限位：`rwx` 与八进制

每一位的含义：

| 位 | 对文件 | 对目录 |
|---|---|---|
| `r` (4) | 读内容 | 列出目录项名字（`ls`） |
| `w` (2) | 修改内容 | 创建/删除/重命名目录项（还需要 `x`） |
| `x` (1) | 执行程序/脚本 | 进入/遍历目录（`cd`、访问 `dir/file`） |

把每三个一组按 `r=4, w=2, x=1` 相加：

| 符号 | owner | group | other | 八进制 |
|---|---:|---:|---:|---:|
| `rwxr-xr-x` | 7 | 5 | 5 | `755` |
| `rw-r--r--` | 6 | 4 | 4 | `644` |
| `rw-------` | 6 | 0 | 0 | `600` |
| `rwx------` | 7 | 0 | 0 | `700` |
| `rwxrwxrwx` | 7 | 7 | 7 | `777` |

口算 `rwxr-xr-x`：

- owner：`rwx` = 4+2+1 = 7
- group：`r-x` = 4+0+1 = 5
- other：`r-x` = 4+0+1 = 5
- 所以是 `755`。

实际修改：

```bash
chmod 600 file.txt
stat -c '%a %A %n' file.txt
chmod 755 subdir
stat -c '%a %A %n' subdir
```

输出：

```text
600 -rw------- file.txt
755 drwxr-xr-x subdir
```

- `chmod 600 file.txt`：只有 owner 能读写，group/other 无权限。
- `chmod 755 subdir`：owner 可读写进入，group/other 可读和进入，但不能在目录里创建/删除文件。
- `chmod` 的数字模式是“设置成”，不是“增加”；需要增加/删除单个位可以用符号模式，例如 `chmod g+w file.txt`、`chmod o-r file.txt`。

#### 目录的 `x` 为什么是“进入/遍历”

- 目录本身不是可执行文件，目录的 `x` 不是“运行目录”，而是**允许路径解析走到这个目录里**。
- 没有 `x`：即使里面的文件是 `777`，`cat dir/file` 也会失败，因为访问 `dir/file` 需要先“穿过” `dir`。
- 有 `x` 没有 `r`：可以访问已知名字的文件（`cat dir/file`），但不能列出目录内容（`ls dir` 失败）。
- 有 `r` 没有 `x`：能列出名字，但通常不能 `cd` 进去，也不能访问目录项；`ls -l` 可能列出名字但无法 stat 元数据。
- 删除/重命名文件看的是**目录的 `w`+`x`**，不是文件本身的 `w`。文件是 `444` 也可能被删除，只要目录可写（没有 sticky bit 等额外限制时）。

#### `umask`

```bash
umask
umask -S
```

实际输出：

```text
0022
u=rwx,g=rx,o=rx
```

- `umask` 是“创建新文件/目录时默认去掉的权限位”，不是直接设置权限。
- 新文件默认最大权限是 `666`（不会有执行位），新目录默认最大权限是 `777`。
- `umask 022` 时：
  - 新文件：`666 & ~022 = 644` → `rw-r--r--`
  - 新目录：`777 & ~022 = 755` → `rwxr-xr-x`
- `umask 077` 会让新文件变成 `600`、新目录变成 `700`。
- `umask` 只影响之后创建的对象，不改已有文件；它是进程属性，子进程继承。

### 5.1.4 `df` 与 `du`

```bash
df -h
du -sh .
```

`df -h` 的实际输出（节选）：

```text
Filesystem      Size  Used Avail Use% Mounted on
/dev/sdd       1007G  2.6G  954G   1% /
C:\             200G  132G   68G  67% /mnt/c
D:\             367G  229G  138G  63% /mnt/d
```

- `df` 看**文件系统**级别的容量：Size/Used/Avail/Use%/挂载点；`-h` 用 GiB/MiB 显示。
- `Avail` 不等于 `Size - Used`，因为文件系统有保留块（ext4 默认给 root 留 5%）。

```text
20K	.
```

- `du` 看**目录树**实际占用的磁盘块；`-s` 只输出汇总，`-h` 人类可读。
- `du` 统计的是块占用，不是 `stat` 的字节数；稀疏文件、硬链接、块大小都会影响结果。
- `df` 是“整个分区还剩多少”，`du` 是“这个目录用了多少”。

### 5.1.5 `find`、`grep`、`wc`

```bash
find . -name '*.cpp'
grep -rn "TODO" .
ls | wc -l
```

实际输出：

```text
./subdir/other.cpp
./subdir/todo.cpp
```

```text
./file.txt:2:TODO: fix me
./subdir/todo.cpp:1:TODO: second
```

```text
3
```

- `find . -name '*.cpp'`：从当前目录递归查找 basename 匹配 `*.cpp` 的文件；`'*.cpp'` 要加引号，防止 shell 先展开。
- `grep -rn "TODO" .`：`-r` 递归，`-n` 显示行号；输出格式是 `文件:行号:内容`。
- `grep` 找到返回 0，找不到返回 1，出错返回 2；脚本里配合 `set -e` 要注意。
- `ls | wc -l`：`ls` 在管道里通常一行一个名字，`wc -l` 数行数；隐藏文件不包含，目录也算一个条目。文件名里含换行会让这个数字不准。

#### `find -exec` 与 `xargs`

```bash
find . -name '*.cpp' -exec wc -l {} +
find . -name '*.cpp' -print0 | xargs -0 wc -l
```

两者输出相同：

```text
 1 ./subdir/other.cpp
 1 ./subdir/todo.cpp
 2 total
```

- `-exec wc -l {} +`：`{}` 是找到的文件名占位符，`+` 表示把多个文件批量传给一次 `wc`；`\;` 则是每个文件调用一次。
- `-print0` 用 NUL 分隔文件名，`xargs -0` 按 NUL 解析；文件名里有空格、换行时也安全。
- 不用 `-print0` 的 `find ... | xargs` 在文件名含空格时会拆错；这是常见坑。
- `xargs` 也会自动分批，避免命令行过长。

### 5.1.6 重定向与管道

| 写法 | 作用 |
|---|---|
| `>` | stdout 写入文件，**覆盖** |
| `>>` | stdout **追加**到文件 |
| `2>` | stderr 写入文件（覆盖） |
| `&>` | stdout 和 stderr 都写入文件（bash 扩展） |
| `2>&1` | 把 stderr 接到“当前 stdout”所在的位置 |
| `|` | 左边进程的 stdout 接到右边进程的 stdin |

#### `>` 与 `>>`

```bash
printf 'first\n' > out.txt
printf 'second\n' >> out.txt
cat -n out.txt
```

实际输出：

```text
     1	first
     2	second
```

- 第一个 `>` 创建/截断文件；第二个 `>>` 保留原内容并追加。
- `>` 等价于 `O_WRONLY|O_CREAT|O_TRUNC`；`>>` 等价于 `O_WRONLY|O_CREAT|O_APPEND`。

#### `2>`：只重定向 stderr

```bash
ls file.txt no_such_file > only_out.txt 2> only_err.txt
cat only_out.txt
cat only_err.txt
```

实际输出：

```text
file.txt
```

```text
ls: cannot access 'no_such_file': No such file or directory
```

- `>` 只接 stdout；`2>` 只接 stderr。
- 文件描述符：`0=stdin`、`1=stdout`、`2=stderr`；`2>` 就是“把 fd 2 重定向到文件”。

#### `&>`：stdout + stderr 一起

```bash
ls file.txt no_such_file &> both2.txt
cat both2.txt
```

实际输出：

```text
ls: cannot access 'no_such_file': No such file or directory
file.txt
```

- `&>` 是 bash 的简写，等价于 `> both2.txt 2>&1`。
- 两条流合并后，**先后顺序不保证**：stderr 通常不带缓冲，stdout 可能带缓冲，所以这里 stderr 先出现。
- 注意 `2>&1 > file` 和 `> file 2>&1` 顺序不同：前者先把 stderr 接到“旧的 stdout”（通常是终端），再把 stdout 改到文件；后者才是两者都进文件。

#### 管道 `|`

```bash
printf 'hello pipe\n' | wc -c
```

实际输出：

```text
11
```

- `|` 只连接 stdout；左边 stderr 仍然输出到终端。
- 想把 stderr 也送进管道：`cmd 2>&1 | ...`。
- `wc -c` 数字节，`hello pipe\n` 是 11 字节（含换行）。

### 5.1.7 `ps aux | grep`

```bash
ps aux | grep '[s]leep 300'
```

实际输出：

```text
chengyu+    2023  0.0  0.0   3128  1600 pts/2    S    23:38   0:00 sleep 300
```

- `ps aux` 是 BSD 风格：USER、PID、%CPU、%MEM、VSZ、RSS、TTY、STAT、START、TIME、COMMAND。
- `grep '[s]leep 300'` 里的 `[s]` 是技巧：grep 自己的命令行是 `grep [s]leep 300`，不匹配 `sleep 300`，所以不会把自己列出来。
- 不加方括号时，`ps aux | grep sleep` 通常会多出一行 grep 自己。

## 5.2 进程与信号

### 5.2.1 后台进程与 `ps`

```bash
sleep 300 &
pid=$!
ps -o pid,ppid,stat,cmd -p "$pid"
ps aux | grep '[s]leep 300'
ps -ef | grep '[s]leep 300'
```

实际输出：

```text
    PID    PPID STAT CMD
   2023    2022 S    sleep 300
```

```text
chengyu+    2023  0.0  0.0   3128  1600 pts/2    S    23:38   0:00 sleep 300
```

```text
chengyu+    2023    2022  0 23:38 pts/2    00:00:00 sleep 300
```

- `&` 把命令放到后台，shell 立刻返回提示符；`$!` 是最后一个后台进程的 PID。
- `ps -o pid,ppid,stat,cmd -p PID`：只看指定 PID，列可以自己选。
  - `PID` 进程号；`PPID` 父进程号（这里是运行脚本的 shell）；`STAT` 状态；`CMD` 命令。
- `ps aux` 与 `ps -ef` 是两种常见格式，列名不同，信息本质相同。
- `STAT` 常见值：
  - `R` running/runnable
  - `S` sleeping（可中断睡眠）
  - `D` uninterruptible sleep（通常等 I/O，见 5.2.7）
  - `T` stopped（被 SIGTSTP/SIGSTOP 停止）
  - `Z` zombie（已退出但父进程还没 `wait`）
  - `+` 在前台进程组；`s` session leader；`l` 多线程等

### 5.2.2 `/proc/<pid>`

```bash
grep -E '^(Name|State|Pid|PPid|Uid|Gid|Threads|SigIgn|SigCgt):' /proc/$pid/status
tr '\0' ' ' < /proc/$pid/cmdline; echo
```

实际输出：

```text
Name:	sleep
State:	S (sleeping)
Pid:	2023
PPid:	2022
Uid:	1000	1000	1000	1000
Gid:	1000	1000	1000	1000
Threads:	1
SigIgn:	0000000000000006
SigCgt:	0000000000000000
```

```text
sleep 300
```

- `/proc` 是虚拟文件系统，内容来自内核，不占磁盘。
- `status` 里 `State` 是进程状态；`Uid/Gid` 四个数字分别是 real/effective/saved/fs。
- `SigIgn` 是“被忽略的信号”位掩码；`SigCgt` 是“被捕获的信号”位掩码。十六进制每一位对应一个信号，bit 0 是信号 1（SIGHUP），bit 1 是信号 2（SIGINT）。
- `0x6` = bit 1 + bit 2 = SIGINT + SIGQUIT 被忽略。为什么后台 `sleep` 会忽略它们？见 5.2.5。
- `cmdline` 是 NUL 分隔的 argv，`tr '\0' ' '` 把它变成可读的一行。

### 5.2.3 常见信号

```bash
kill -l | head -n 4
for n in 1 2 9 13 15 18 19 20; do printf '%s = ' "$n"; kill -l "$n"; done
```

实际输出：

```text
 1) SIGHUP	 2) SIGINT	 3) SIGQUIT	 4) SIGILL	 5) SIGTRAP
 6) SIGABRT	 7) SIGBUS	 8) SIGFPE	 9) SIGKILL	10) SIGUSR1
11) SIGSEGV	12) SIGUSR2	13) SIGPIPE	14) SIGALRM	15) SIGTERM
16) SIGSTKFLT	17) SIGCHLD	18) SIGCONT	19) SIGSTOP	20) SIGTSTP
```

```text
1 = HUP
2 = INT
9 = KILL
13 = PIPE
15 = TERM
18 = CONT
19 = STOP
20 = TSTP
```

| 编号 | 名称 | 默认动作 | 能否捕获/忽略 | 典型场景 |
|---:|---|---|---|---|
| 1 | `SIGHUP` | 终止 | 能 | 终端挂断；`nohup` 忽略它 |
| 2 | `SIGINT` | 终止 | 能 | Ctrl+C |
| 9 | `SIGKILL` | 终止 | **不能** | 强制杀进程 |
| 13 | `SIGPIPE` | 终止 | 能 | 往没有读端的管道写 |
| 15 | `SIGTERM` | 终止 | 能 | 默认的 `kill`，优雅终止 |
| 18 | `SIGCONT` | 继续 | 能 | 恢复被停止的进程 |
| 19 | `SIGSTOP` | 停止 | **不能** | 强制暂停 |
| 20 | `SIGTSTP` | 停止 | 能 | Ctrl+Z |

- `kill -l` 不带参数列出所有信号；`kill -l 15` 打印 `TERM`。
- `kill PID` 默认发送 `SIGTERM`；`kill -TERM PID`、`kill -15 PID` 等价。
- 进程被信号终止时，`wait` 的返回状态通常是 `128 + 信号编号`：TERM → 143，KILL → 137，INT → 130。

### 5.2.4 `SIGTERM` vs `SIGKILL`

```bash
kill -TERM "$pid"
wait "$pid"
echo "wait status=$?"
```

实际输出：

```text
wait status=143
```

```bash
kill -KILL "$pid2"
wait "$pid2"
echo "wait status=$?"
```

实际输出：

```text
wait status=137
```

- `SIGTERM`（15）：默认终止，但进程可以安装 handler 做清理、可以忽略、可以阻塞；适合“请你退出”。
- `SIGKILL`（9）：内核直接终止，不能捕获、不能忽略、不能阻塞；进程没有机会执行 `atexit`/析构/清理逻辑。
- `SIGKILL` 的“强制”也有边界：进程处于 `D`（uninterruptible sleep）时，信号会一直 pending，直到进程从内核态返回或 I/O 完成；这期间 `kill -9` 看起来“杀不掉”。详见 5.2.7。
- 僵尸进程（`Z`）已经死了，`kill` 对它是无效的；需要父进程 `wait` 回收。

### 5.2.5 Ctrl+C、Ctrl+Z、`jobs`、`bg`、`fg`

#### Ctrl+C（SIGINT）

终端里的 Ctrl+C 不是“键盘输入字符”，而是终端驱动向前台进程组发送 `SIGINT`。默认动作是终止。

这里踩到一个真实的坑：**非交互 shell 用 `&` 启动的异步命令，会继承“忽略 SIGINT/SIGQUIT”的处置**。所以直接对后台 `sleep` 发 `kill -INT` 不会结束它：

```text
SigIgn: 0000000000000006   # 0x6 = SIGINT + SIGQUIT 被忽略
```

为了模拟“前台进程收到 Ctrl+C”，实验里用 `env --default-signal=INT` 恢复默认处置：

```bash
env --default-signal=INT sleep 300 &
pid3=$!
kill -INT "$pid3"
```

实际结果：

```text
State:	S (sleeping)
SigIgn:	0000000000000004
terminated by SIGINT
wait status=130
```

- `0x4` 只剩 SIGQUIT 被忽略，SIGINT 已恢复默认；`kill -INT` 成功终止，`wait` 返回 130。
- 交互式 shell 有 job control 时，Ctrl+C 发给前台进程组，普通前台程序会正常收到 SIGINT；上面的坑主要出现在“非交互脚本 + 后台任务”。

#### Ctrl+Z（SIGTSTP）与 `jobs` / `bg` / `fg`

Ctrl+Z 发送 `SIGTSTP`（20），默认动作是**停止**（不是终止）。shell 拿回提示符，任务进入 `T` 状态。

```bash
set -m
sleep 30 &
j=$!
jobs -l
kill -TSTP "$j"
jobs -l
ps -o pid,stat,cmd -p "$j"
bg %1
jobs -l
ps -o pid,stat,cmd -p "$j"
```

实际输出：

```text
[1]+  2003 Running                 sleep 30 &
[1]+  2003 Stopped                 sleep 30
    PID STAT CMD
   2003 T    sleep 30
[1]+ sleep 30 &
[1]+  2003 Running                 sleep 30 &
    PID STAT CMD
   2003 S    sleep 30
```

- `jobs -l`：`[1]` 是 job 号，`2003` 是 PID，`Running`/`Stopped` 是状态。
- `bg %1`：让 1 号 job 在后台继续运行（相当于发送 `SIGCONT` 并放到后台）。
- `fg %1`：把 1 号 job 调到前台并等待它结束。

`fg` 的实际输出（用一个 2 秒的 sleep）：

```text
[1]+  2008 Running                 sleep 2 &
sleep 2
fg status=0
```

- Ctrl+Z 后 shell 提示符回来，但任务没有死；`jobs` 能看到它。
- `SIGSTOP`（19）和 `SIGTSTP` 都会停止进程，但 `SIGSTOP` 不能被捕获/忽略，类似“停止版 SIGKILL”。
- `SIGCONT`（18）恢复被停止的进程。

### 5.2.6 `nohup` 与 `disown`

```bash
nohup sleep 300 >/tmp/d02_nohup.out 2>&1 &
npid=$!
grep -E '^(State|SigIgn):' /proc/$npid/status
kill -HUP "$npid"
ps -o pid,ppid,stat,cmd -p "$npid"
```

实际输出：

```text
State:	S (sleeping)
SigIgn:	0000000000000007
    PID    PPID STAT CMD
   2044    2022 S    sleep 300
```

- `nohup` 让进程忽略 `SIGHUP`（终端挂断），所以 `SigIgn` 从 `0x6` 变成 `0x7`（多了 bit 0 = SIGHUP）。
- `kill -HUP` 之后进程仍然活着；最后用 `kill -TERM` 结束。
- `nohup` 在 stdout 是终端时，默认把输出写到 `nohup.out`；这里显式重定向到 `/tmp/d02_nohup.out`。
- `&` 只是后台运行，进程仍在 shell 的 job 表里；终端关闭时可能收到 SIGHUP。
- `disown` 把 job 从 shell 的 job 表移除，shell 退出时不会再向它发 SIGHUP；常见写法：

```bash
sleep 300 &
disown
```

- `nohup` 和 `disown` 解决的角度不同：`nohup` 改信号处置并重定向输出；`disown` 改 shell 的 job 管理。需要完全脱离终端会话还可以用 `setsid`，今天不展开。

### 5.2.7 为什么 `SIGKILL` 杀不掉 D 状态进程

- `D` = uninterruptible sleep，通常是在内核里等待 I/O（磁盘、NFS、驱动等）完成。
- 信号（包括 SIGKILL）通常要在进程从内核态返回用户态、或进入可中断睡眠时才会被处理；D 状态不可中断，SIGKILL 只能保持 pending。
- 所以 `kill -9` 不是“立刻从 CPU 上抹掉进程”，它只是设置一个不可忽略的 pending 信号；进程要能被唤醒并走到信号处理点才会死。
- 常见解决办法：等 I/O 完成/超时、修复底层存储或网络、必要时重启；不要在生产环境随手 `kill -9` 掩盖 D 状态问题。

## 5.3 strace 入门

### 5.3.1 环境说明

本机 WSL 默认没有安装 `strace`，而且 `sudo` 需要密码。本次没有改系统包，而是：

```bash
cd /tmp
apt-get download strace
dpkg-deb -x strace_6.8-0ubuntu2_amd64.deb /tmp/strace-root
/tmp/strace-root/usr/bin/strace -V
```

```text
strace -- version 6.8
```

下面的命令用完整路径 `/tmp/strace-root/usr/bin/strace`；如果你的系统已经装了 `strace`，直接写 `strace` 即可。

被跟踪的程序是 D01 的 `hello`：

```bash
cd /mnt/d/Project/CPP/CPPLearn/base/days/d01-toolchain
./hello
```

```text
Hello, World!
```

### 5.3.2 过滤 syscall：`openat,read,write,exit_group`

```bash
strace -f -e trace=openat,read,write,exit_group ./hello
```

实际输出（12 行）：

```text
1595  openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
1595  openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libstdc++.so.6", O_RDONLY|O_CLOEXEC) = 3
1595  read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\0\0\0\0\0\0\0\0"..., 832) = 832
1595  openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
1595  read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\220\243\2\0\0\0\0\0"..., 832) = 832
1595  openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libm.so.6", O_RDONLY|O_CLOEXEC) = 3
1595  read(3, "\177ELF\2\1\1\3\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\0\0\0\0\0\0\0\0"..., 832) = 832
1595  openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libgcc_s.so.1", O_RDONLY|O_CLOEXEC) = 3
1595  read(3, "\177ELF\2\1\1\0\0\0\0\0\0\0\0\0\3\0>\0\1\0\0\0\0\0\0\0\0\0\0\0"..., 832) = 832
1595  write(1, "Hello, World!\n", 14)   = 14
1595  exit_group(0)                     = ?
1595  +++ exited with 0 +++
```

逐行解释：

- `-f`：跟随 fork/vfork/clone 产生的子进程和线程；`hello` 是单线程，但这是常用选项。
- `-e trace=openat,read,write,exit_group`：只显示这四类 syscall，其他全部过滤掉。
- `openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3`：
  - `AT_FDCWD` 表示相对当前工作目录解析路径；
  - `O_RDONLY` 只读，`O_CLOEXEC` 执行 `exec` 时自动关闭；
  - 返回值 `3` 是新的文件描述符（0/1/2 已被 stdin/stdout/stderr 占用）。
- `read(3, "\177ELF...", 832) = 832`：从 fd 3 读 832 字节，开头 `\177ELF` 是 ELF 魔数；动态链接器在读取共享库的 ELF 头。
- 打开的共享库依次是 `libstdc++.so.6`、`libc.so.6`、`libm.so.6`、`libgcc_s.so.1`。
- `write(1, "Hello, World!\n", 14) = 14`：
  - `1` 是 stdout 的文件描述符；
  - 写入 14 字节（`Hello, World!` 13 字节 + 换行 1 字节）；
  - 返回值 14 表示成功写入的字节数。
- `exit_group(0)`：退出整个线程组，退出码 0；`?` 表示该 syscall 不返回（进程结束）。

#### 动态链接器和 libc 在哪里

`hello` 是动态链接的，ELF 头里指定了 interpreter：

```bash
ldd ./hello
readelf -l ./hello | grep -A1 interpreter
```

```text
	linux-vdso.so.1 (0x00007ffd937b6000)
	libstdc++.so.6 => /lib/x86_64-linux-gnu/libstdc++.so.6 (...)
	libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (...)
	libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (...)
	/lib64/ld-linux-x86-64.so.2 (...)
	libgcc_s.so.1 => /lib/x86_64-linux-gnu/libgcc_s.so.1 (...)
```

```text
      [Requesting program interpreter: /lib64/ld-linux-x86-64.so.2]
```

- `ld-linux` 是动态链接器。内核在 `execve` 时直接根据 `PT_INTERP` 把它映射进来，所以 `strace` 的用户态 syscall 里通常看不到 `openat("ld-linux...")`。
- strace 里最先出现的用户态文件是 `/etc/ld.so.cache`（动态库缓存），然后是 `libc` 等共享库。
- `linux-vdso.so.1` 是内核映射的虚拟共享对象，没有对应磁盘文件。

### 5.3.3 `strace -c`：统计

```bash
strace -c ./hello
```

实际输出：

```text
% time     seconds  usecs/call     calls    errors syscall
------ ----------- ----------- --------- --------- ----------------
  0.00    0.000000           0         4           read
  0.00    0.000000           0         1           write
  0.00    0.000000           0         5           close
  0.00    0.000000           0         6           fstat
  0.00    0.000000           0        22           mmap
  0.00    0.000000           0         6           mprotect
  0.00    0.000000           0         1           munmap
  0.00    0.000000           0         3           brk
  0.00    0.000000           0         2           pread64
  0.00    0.000000           0         1         1 access
  0.00    0.000000           0         1           execve
  0.00    0.000000           0         1           arch_prctl
  0.00    0.000000           0         1           futex
  0.00    0.000000           0         1           set_tid_address
  0.00    0.000000           0         5           openat
  0.00    0.000000           0         1           set_robust_list
  0.00    0.000000           0         1           prlimit64
  0.00    0.000000           0         1           getrandom
  0.00    0.000000           0         1           rseq
------ ----------- ----------- --------- --------- ----------------
100.00    0.000000           0        64         1 total
```

列含义：

- `% time`：该 syscall 占总时间的百分比；程序太快，全部显示 0.00。
- `seconds`：总耗时；`usecs/call`：平均每次耗时（微秒）。
- `calls`：调用次数；`errors`：失败次数（返回负 errno 的次数）。
- 总计：64 次 syscall，1 次失败。
- 唯一的失败是 `access("/etc/ld.so.preload", R_OK) = -1 ENOENT`：动态链接器检查可选的预加载文件，文件不存在是正常情况。
- `mmap` 22 次、`openat` 5 次、`read` 4 次，反映了动态链接和加载共享库的过程。
- `-c` 不打印每次调用的参数和返回值，只做汇总；适合快速判断“哪个 syscall 调用最多/最慢/失败最多”。

### 5.3.4 默认 `strace` 与 `-c` 的差异

```bash
strace ./hello          # 默认：逐条打印所有 syscall
strace -c ./hello       # 汇总：按 syscall 统计
```

本机结果：

- 默认 `strace ./hello` 输出 66 行，包含 `execve`、`brk`、`mmap`、`access`、`openat`、`fstat`、`read`、`write`、`exit_group` 等每一条调用及参数/返回值。
- `strace -c` 只输出约 20 行统计表，适合看总量；具体每次调用仍要看默认模式或 `-e trace=` 过滤。

默认模式的开头几行：

```text
execve("./hello", ["./hello"], 0x7fff1f778030 /* 24 vars */) = 0
brk(NULL)                               = 0x5cdabd4c0000
mmap(NULL, 8192, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0) = 0x77a145b25000
access("/etc/ld.so.preload", R_OK)      = -1 ENOENT (No such file or directory)
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
...
```

结尾：

```text
fstat(1, {st_mode=S_IFIFO|0600, st_size=0, ...}) = 0
write(1, "Hello, World!\n", 14)         = 14
exit_group(0)                           = ?
+++ exited with 0 +++
```

- `execve` 是程序启动的第一个 syscall；`brk`/`mmap` 在建立堆和映射；`fstat(1, ...)` 检查 stdout 的类型。
- `write(1, ...)` 再次确认 fd 1 是 stdout。
- 默认模式信息量大，定位“打开了哪些文件”时应加 `-e trace=openat` 或 `-e trace=file`。

### 5.3.5 用 `strace` 定位文件

```bash
strace -e trace=openat ./hello
```

等价于只看 `openat`（也可以写 `-e trace=file`，会包含 `open`、`openat`、`stat` 等文件相关 syscall）。本机输出 5 行：

```text
openat(AT_FDCWD, "/etc/ld.so.cache", O_RDONLY|O_CLOEXEC) = 3
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libstdc++.so.6", O_RDONLY|O_CLOEXEC) = 3
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libc.so.6", O_RDONLY|O_CLOEXEC) = 3
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libm.so.6", O_RDONLY|O_CLOEXEC) = 3
openat(AT_FDCWD, "/lib/x86_64-linux-gnu/libgcc_s.so.1", O_RDONLY|O_CLOEXEC) = 3
```

- 用 `-e trace=openat` 可以快速回答“程序启动时打开了哪些文件”。
- 如果要看子进程/线程，再加 `-f`。
- 如果程序已经跑起来，可以 `strace -p PID` attach；这需要 ptrace 权限，容器/`ptrace_scope` 可能限制。

## 5.4 易错点

1. **权限位不是“三个数字随便写”**：`755` 是 owner 7、group 5、other 5；`644` 是 6/4/4。先按三元组拆开再算。
2. **目录的 `x` 不是“执行目录”**：是 traverse/enter；没有 `x`，即使文件 `777` 也访问不到。
3. **目录的 `r` 和 `x` 是两件事**：`r` 能列名字，`x` 能进入/按名字访问；有 `r` 无 `x` 通常不能 `cd`。
4. **删除文件看目录权限**：文件本身 `444` 也可能被删除，只要目录对你有 `w`+`x`。
5. **`>` 会截断**：想保留原内容用 `>>`；`>` 重定向的是 stdout，不是 stderr。
6. **`2>` 只接 stderr**：stdout 仍去终端；`&>` 才是两者都接（bash）。
7. **`2>&1 > file` 顺序是错的**：先把 stderr 接到旧 stdout，再把 stdout 改到文件；要写 `> file 2>&1`。
8. **管道只接 stdout**：stderr 不会自动进管道；需要 `2>&1 |`。
9. **`grep` 自己会出现在 `ps | grep` 里**：用 `[s]leep` 这种方括号技巧，或 `pgrep -af`。
10. **后台异步命令可能忽略 SIGINT**：非交互 shell 的 `&` 任务常带 `SigIgn=...0006`；`kill -INT` 无效不是信号坏了。
11. **`SIGTERM` 可捕获，`SIGKILL` 不可捕获**：前者允许清理，后者由内核强制；`kill -9` 也不是对 D 状态万能。
12. **`SIGKILL` 不会执行清理**：析构、`atexit`、临时文件删除都可能没机会跑。
13. **`nohup` 不等于完全 daemon**：它忽略 SIGHUP 并重定向输出；要脱离会话还需要 `setsid` 等。
14. **`strace -c` 的时间可能全是 0**：短程序/快 syscall 统计精度不够；看 `calls`/`errors` 更可靠。
15. **`strace` 会拖慢程序**：ptrace 每次 syscall 都停一下；不要用它做性能基准。

## 5.5 待复习

- `rwx` → 八进制口算；`755`、`644`、`600`、`700`、`777` 各是什么。
- 目录的 `r`、`w`、`x` 分别允许什么；为什么删除文件看目录权限。
- `umask` 如何从 `666`/`777` 算出新文件/新目录权限。
- `>`、`>>`、`2>`、`&>`、`2>&1`、`|` 的区别和顺序。
- `find -exec ... {} +` 与 `-print0 | xargs -0` 的用途。
- `ps -o pid,ppid,stat,cmd -p`、`ps aux`、`ps -ef` 的列。
- `/proc/<pid>/status` 和 `/proc/<pid>/cmdline` 能看什么。
- `SIGINT`、`SIGTERM`、`SIGKILL`、`SIGHUP`、`SIGPIPE`、`SIGTSTP`、`SIGSTOP`、`SIGCONT` 的编号和默认动作。
- `SIGTERM` vs `SIGKILL`；为什么 `SIGKILL` 杀不掉 D 状态。
- Ctrl+C / Ctrl+Z 分别发什么信号；`jobs`、`bg`、`fg` 怎么配合。
- `nohup` 和 `disown` 的区别。
- `strace -e trace=openat` 怎么定位文件；`write(1, ...)` 的 `1` 是什么。
- `strace` 默认输出与 `strace -c` 的差异；`calls`/`errors` 两列的含义。
