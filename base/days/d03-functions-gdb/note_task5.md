# D03 任务 5：Git 分支与合并

> 目标：会用分支隔离开发，会把分支合并回 `main`，遇到冲突知道怎么解决。
>
> 下面只保留日常最常用的命令。`fast-forward`、`--no-ff` 和冲突输出在临时仓库里验证过；
> 实际项目中的提交信息和文件路径按自己的来。

## 0. 日常 Git 命令速查

### 0.1 最常用的一条链

```bash
git status                 # 现在改了什么
git diff                   # 具体改了什么
git add <file>             # 把改动放进暂存区
git commit -m "说明"        # 提交
git log --oneline --graph --all   # 看提交历史
```

记忆顺序：

```text
status → diff → add → commit → log
```

> **记忆点：** 日常 Git 五连是 `status → diff → add → commit → log`；先看状态，再决定提交什么。

### 0.2 常用命令表

| 分类 | 命令 | 作用 |
|---|---|---|
| 查看状态 | `git status` | 查看当前分支、暂存区、未跟踪文件 |
| 查看状态 | `git status -sb` | 简短格式，适合频繁查看 |
| 查看差异 | `git diff` | 工作区 vs 暂存区 |
| 查看差异 | `git diff --staged` | 暂存区 vs 最后一次提交 |
| 查看差异 | `git diff HEAD` | 工作区 + 暂存区 vs 最后一次提交 |
| 暂存 | `git add <file>` | 暂存指定文件 |
| 暂存 | `git add .` | 暂存当前目录下的改动 |
| 暂存 | `git add -A` | 暂存所有改动（含删除） |
| 提交 | `git commit -m "msg"` | 提交暂存区 |
| 提交 | `git commit -am "msg"` | 暂存**已跟踪**文件的修改并提交；不包含新文件 |
| 历史 | `git log --oneline` | 一行一条提交 |
| 历史 | `git log --oneline --graph --all` | 带分支图看所有分支 |
| 历史 | `git show HEAD` | 查看最后一次提交的内容 |
| 分支 | `git branch` | 列出本地分支 |
| 分支 | `git branch -d <name>` | 删除已合并的分支 |
| 分支 | `git branch -D <name>` | 强制删除分支（未合并也会删，慎用） |
| 切换 | `git switch main` | 切到 `main` |
| 切换 | `git switch -c <name>` | 新建并切换到分支 |
| 合并 | `git merge <branch>` | 把指定分支合并到当前分支 |
| 合并 | `git merge --no-ff <branch>` | 强制生成 merge commit，保留分支历史 |
| 合并 | `git merge --abort` | 放弃本次合并，回到合并前 |
| 变基 | `git rebase main` | 把当前分支的提交搬到 `main` 最新提交之后 |
| 变基 | `git rebase --continue` | 解决冲突后继续 rebase |
| 变基 | `git rebase --abort` | 放弃整个 rebase |
| 撤销 | `git restore <file>` | 丢弃工作区改动（危险） |
| 撤销 | `git restore --staged <file>` | 取消暂存，保留工作区改动 |
| 撤销 | `git reset --soft HEAD~1` | 撤销最后一次提交，改动仍在暂存区 |
| 撤销 | `git reset --hard HEAD~1` | 撤销提交并丢弃改动（危险） |
| 撤销 | `git revert <commit>` | 生成一个新提交来抵消旧提交，适合已推送的历史 |
| 暂存现场 | `git stash` | 临时保存未提交改动 |
| 恢复现场 | `git stash pop` | 恢复最近一次 stash |
| 远程 | `git fetch origin` | 拉取远程更新，不改工作区 |
| 远程 | `git pull` | 拉取并合并远程更新 |
| 远程 | `git push` | 推送本地提交到远程 |
| 找回 | `git reflog` | 查看 HEAD 移动记录，找回误删提交 |

> **记忆点：** `restore` 改工作区，`restore --staged` 只取消暂存，`reset` 会动提交历史；危险命令先看清再执行。

## 1. 分支开发流程

任务 5 的完整流程：

```bash
# 1. 新建并切换到 D03 分支
git switch -c d03-functions-gdb

# 2. 查看改动
git status
git diff

# 3. 暂存并提交
git add base/days/d03-functions-gdb base/CMakeLists.txt
git commit -m "d03: 函数、链接与 GDB"

# 4. 切回 main
git switch main

# 5. 合并，强制保留 merge commit
git merge --no-ff d03-functions-gdb -m "merge: d03 函数、链接与 GDB"

# 6. 看分支图
git log --oneline --graph --all

# 7. 删除已经合并的分支
git branch -d d03-functions-gdb
```

`git switch -c` 等价于旧写法：

```bash
git checkout -b d03-functions-gdb
```

新版本 Git 推荐用 `switch` 专门切分支，`restore` 专门恢复文件，语义更清楚。

> **记忆点：** 分支只是一个指向提交的指针；`switch -c` 新建分支，`merge` 把另一条指针的历史并进来。

## 2. fast-forward 与 `--no-ff`

### 2.1 默认 fast-forward

如果 `main` 在分支创建后没有新提交，合并时 Git 默认直接“快进”指针：

```text
$ git merge d03-functions-gdb
Updating 6314547..11adff6
Fast-forward
 f.txt | 1 +
 1 file changed, 1 insertion(+)

$ git log --oneline --graph --all
* 11adff6 d03: work
* 6314547 base
```

历史是一条直线，看不出“曾经有过一个分支”。

### 2.2 `--no-ff` 保留分支痕迹

```text
$ git merge --no-ff d03-functions-gdb -m "merge: d03"
Merge made by the 'ort' strategy.
 f.txt | 1 +
 1 file changed, 1 insertion(+)

$ git log --oneline --graph --all
*   50b507d merge: d03
|\
| * d5a1682 d03: work
|/
* c9c3d5b base
```

多出来的 `merge: d03` 就是 merge commit，能看出：

- 这里曾经有一个分支；
- 分支上的提交是哪些；
- 什么时候合并回 `main`。

| 方式 | 历史形状 | 适用 |
|---|---|---|
| fast-forward | 一条直线 | 个人小改动，不在意分支痕迹 |
| `--no-ff` | 有分叉和 merge commit | 功能分支、需要保留“合并过一个分支”的记录 |

> **记忆点：** fast-forward 只移动指针，`--no-ff` 多一个 merge commit；想保留分支历史就用 `--no-ff`。

## 3. git merge 与 git rebase 的区别

### 3.1 一句话区别

- `git merge`：保留两条分支的历史，再生成一个 merge commit 把它们接起来。
- `git rebase`：把自己分支上的提交“搬”到目标分支的最新提交之后，历史变成一条直线。

### 3.2 merge 的效果

假设 `main` 和 `feature` 各自都有新提交：

```bash
git switch main
git merge --no-ff feature -m "merge: feature"
```

历史：

```text
*   34f5c62 merge: feature
|\
| * 6f2dd1b feature: 2
| * 2b71935 feature: 1
* | 4b58f51 main: 1
|/
* 1a28d6b base
```

特点：

- `feature` 的两个提交原样保留；
- 多出一个 merge commit `34f5c62`；
- 能看出 `feature` 是从 `base` 分出去的，也能看出它什么时候合回来。

### 3.3 rebase 的效果

同样是上面这个仓库，在 `feature` 上执行：

```bash
git switch feature
git rebase main
```

实际输出：

```text
Rebasing (1/2)...
Rebasing (2/2)...
Successfully rebased and updated refs/heads/feature.
```

rebase 前：

```text
* 6b71e9d feature: 2
* dc9b99a feature: 1
| * cecc865 main: 1
|/
* 7c8219b base
```

rebase 后：

```text
* 48b932f feature: 2
* 91453bd feature: 1
* cecc865 main: 1
* 7c8219b base
```

特点：

- 历史变成一条直线，看起来像“先做完 `main`，再接着做 `feature`”；
- 没有 merge commit；
- `feature` 的两个提交被重新创建，哈希从 `dc9b99a/6b71e9d` 变成 `91453bd/48b932f`；
- `main` 的提交 `cecc865` 没有变。

> **记忆点：** rebase 不是“移动原提交”，而是把提交内容复制到新基点后重新提交，所以哈希会变。

### 3.4 对比表

| 维度 | `git merge` | `git rebase` |
|---|---|---|
| 历史形状 | 保留分叉，有 merge commit | 一条直线，没有 merge commit |
| 原提交哈希 | 不变 | 被复制后生成新哈希 |
| 是否新增提交 | 通常新增一个 merge commit | 不新增 merge commit，但重写原提交 |
| 冲突 | 合并时统一解决一次 | 可能每个被复制的提交都要解决一次 |
| 是否改写历史 | 不改写 | 改写 |
| 适合 | 公共分支、功能分支合并、保留真实上下文 | 本地整理提交、把 `main` 新提交同步到功能分支 |
| 风险 | 历史分叉多时图会比较乱 | 对已推送/多人共用的分支 rebase 会破坏别人的历史 |

### 3.5 使用原则

推荐做法：

```bash
# 功能分支开发期间，用 rebase 跟上 main
git switch feature
git rebase main

# 功能完成，合并回 main 时用 merge
git switch main
git merge --no-ff feature -m "merge: feature"
```

或者：

```bash
# 功能分支先 rebase 到 main 最新提交
git switch feature
git rebase main

# 回 main 直接快进合并，得到一条直线
git switch main
git merge feature
```

核心原则：

- **公共分支不要 rebase**：`main`、`develop` 这类别人也在用的分支，用 `merge`；
- **自己的本地功能分支可以 rebase**：用来整理提交、同步 `main`；
- **已经推送的提交尽量别 rebase**：rebase 后哈希变了，别人基于旧提交的工作会乱；
- 如果确实要推送到远程的个人分支，用：

```bash
git push --force-with-lease
```

不要用裸 `--force`，`--force-with-lease` 会在远程被别人更新过时拒绝覆盖。

> **记忆点：** 公共分支用 `merge`，自己的本地分支才用 `rebase`；rebase 后哈希会变，已推送的共享分支不要 rebase。

### 3.6 rebase 遇到冲突

rebase 过程中也可能冲突：

```bash
git rebase main
# 冲突后：
git status
# 手工改文件，删掉冲突标记
git add <file>
git rebase --continue
```

其他选择：

```bash
git rebase --abort    # 放弃整个 rebase，回到开始前
git rebase --skip     # 跳过当前这个提交（会丢掉该提交的改动，慎用）
```

和 merge 的区别：

- merge 冲突通常只需要解决一次，然后 `git commit`；
- rebase 是逐个提交重放，可能在多个提交上分别停下来解决冲突；
- rebase 解决冲突后用 `git rebase --continue`，不是 `git commit`。

> **记忆点：** merge 冲突解决一次后 `commit`；rebase 冲突逐个提交解决，用 `--continue` 继续，不想做就 `--abort`。

## 4. 冲突处理

### 4.1 制造冲突

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

实际输出：

```text
Auto-merging merge_practice.txt
CONFLICT (content): Merge conflict in merge_practice.txt
Automatic merge failed; fix conflicts and then commit the result.
```

### 4.2 看冲突状态

```bash
git status
```

实际输出：

```text
On branch main
You have unmerged paths.
  (fix conflicts and run "git commit")
  (use "git merge --abort" to abort the merge)

Unmerged paths:
  (use "git add <file>..." to mark resolution)
	both modified:   merge_practice.txt

no changes added to commit (use "git add" and/or "git commit -a")
```

简短格式：

```text
$ git status --short
UU merge_practice.txt
```

`UU` 表示两边都修改了同一个文件，Git 不知道该保留哪一个。

### 4.3 冲突标记

打开文件会看到：

```text
<<<<<<< HEAD
main
=======
branch
>>>>>>> d03-conflict
```

| 部分 | 含义 |
|---|---|
| `<<<<<<< HEAD` 到 `=======` | 当前分支（这里是 `main`）的内容 |
| `=======` 到 `>>>>>>> d03-conflict` | 要合并进来的分支内容 |

### 4.4 解决并提交

手工把文件改成最终想要的内容，并删掉三行标记，例如：

```text
main + branch
```

然后：

```bash
git add merge_practice.txt
git commit -m "d03: resolve conflict"
```

`git add` 不只是“暂存”，它同时告诉 Git：这个文件的冲突已经解决。

如果不想继续这次合并：

```bash
git merge --abort
```

如果练习文件不想要了：

```bash
git rm merge_practice.txt
git commit -m "d03: remove merge practice"
```

> **记忆点：** 冲突不是报错，是 Git 让你做决定；改文件 → `git add` → `git commit`，不想合就 `merge --abort`。

## 5. 常见撤销操作

| 场景 | 命令 | 结果 |
|---|---|---|
| 改坏了文件，想丢弃改动 | `git restore <file>` | 工作区恢复到最后一次提交；改动丢失 |
| `git add` 错了，想取消暂存 | `git restore --staged <file>` | 改动还在工作区 |
| 提交信息写错，还没推送 | `git commit --amend -m "新信息"` | 修改最后一次提交 |
| 想撤销最后一次提交，改动保留 | `git reset --soft HEAD~1` | 提交没了，改动在暂存区 |
| 想撤销最后一次提交，改动不暂存 | `git reset HEAD~1` | 提交没了，改动在工作区 |
| 想彻底丢弃最后一次提交和改动 | `git reset --hard HEAD~1` | 危险，改动无法从工作区找回 |
| 已推送的提交想撤销 | `git revert <commit>` | 生成一个反向提交，历史不被改写 |
| 误操作后找提交 | `git reflog` | 查看 HEAD 移动记录，找回丢失的提交 |

`git reset --hard` 和 `git restore <file>` 都会丢改动；执行前先 `git status`、`git diff` 确认。

> **记忆点：** 只取消暂存用 `restore --staged`，只丢工作区用 `restore`，连提交一起动才用 `reset`；已推送的历史用 `revert`。

## 6. 最常用的几条，背下来

```bash
git status
git diff
git add .
git commit -m "msg"
git log --oneline --graph --all
git switch main
git switch -c feature
git merge --no-ff feature
git branch -d feature
git restore --staged file
git restore file
git push
```

> **记忆点：** 每天真正高频的是 `status/diff/add/commit/log`，分支操作则是 `switch -c → merge --no-ff → branch -d`。

## 7. 自测

- `git switch -c` 和 `git switch` 的区别？
- `git add` 和 `git commit` 分别把改动放在哪里？
- `git diff`、`git diff --staged`、`git diff HEAD` 各看什么？
- fast-forward 和 `--no-ff` 的历史有什么区别？
- `git merge` 和 `git rebase` 生成的历史有什么区别？
- 为什么 rebase 后提交哈希会变？
- 为什么公共分支不能随便 rebase？自己的功能分支什么时候适合 rebase？
- rebase 冲突后怎么继续？怎么放弃？
- 冲突时 `<<<<<<<`、`=======`、`>>>>>>>` 分别表示什么？
- 解决冲突后为什么要 `git add`？
- `git restore` 和 `git restore --staged` 的区别？
- `git reset --soft`、`--mixed`、`--hard` 的区别？
- 为什么已推送的历史更推荐 `git revert` 而不是 `git reset`？
