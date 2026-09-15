# Git 使用说明（以本仓库 RM_2 为例）

这份文档只讲一件事：怎么在 **这台电脑或另一台电脑** 上用 Cursor 打开、修改、上传 **你自己的仓库**。

你的仓库：

- GitHub 网页：<https://github.com/encourotacy/RM_2>
- HTTPS 地址：`https://github.com/encourotacy/RM_2.git`
- 默认分支：`main`
- 远端在 Git 里的名字：`origin`（就是上面这个网址的别名）
- 仓库当前多为 **私有**：没登录时别人打不开，你自己 `clone` / `pull` / `push` 也要 Token（第 5、6 节）。改成公开后，别人和另一台电脑 **下载** 会简单很多，**上传** 仍要登录。

本机当前工程目录示例：`/media/star/Data/RM_2`。换电脑后目录可以不同，只要用 `git clone` 下来再在 Cursor 里打开那个文件夹即可。

---

## 1. 几个名字分别是什么

| 名词 | 含义 |
|------|------|
| Git | 装在电脑上的版本管理工具，负责记录每次修改 |
| GitHub | 网上存仓库的网站，你的代码在 `encourotacy/RM_2` |
| 仓库 / 仓库 | 一份带 `.git` 文件夹的工程；打开 Cursor 就是打开这个文件夹 |
| `origin` | 本仓库里给 GitHub 地址起的默认名字，不是第二个仓库 |
| `main` | 当前主分支 |
| `commit` | 一次本地快照（提交） |
| `push` | 把本地提交上传到 GitHub |
| `pull` | 把 GitHub 上别人或另一台电脑推上去的提交拉下来 |
| `clone` | 把 GitHub 上的仓库完整复制到本机一个新文件夹 |

关系可以记成：

```
Cursor 里改文件 → git add → git commit（只在这台电脑）
                              ↓ git push
                         GitHub 上的 RM_2
                              ↓ git pull / git clone
                         另一台电脑上的同一份工程
```

只保存文件、不 `commit`，Git 不记账。只 `commit`、不 `push`，GitHub 和另一台电脑看不到。

---

## 2. 在另一台电脑用 Cursor 编辑本仓库

目标：换地方打开 Cursor，改的仍是 `encourotacy/RM_2`，而不是误连别人的库。

### 2.1 那台电脑要有的东西

1. 安装 Git、Cursor
2. 能访问 GitHub（国内通常要开代理；见第 4 节）
3. GitHub 账号 `encourotacy` 已登录，或已把该账号加成仓库协作者
4. 仓库若仍是私有：按第 5 节用 Token；若已改成公开：clone 可不登录，push 仍要 Token（第 6 节）

### 2.2 克隆到本机（只做一次）

**不要**把原来的 `origin` 改成别的地址。新开一个空目录，克隆你自己的库：

```bash
cd 你想放代码的地方
git clone https://github.com/encourotacy/RM_2.git
cd RM_2
```

会得到文件夹 `RM_2/`，里面有 `Class3.md`、`class_3/` 等，以及隐藏的 `.git/`。

确认远端是你的库：

```bash
git remote -v
```

应显示：

```text
origin  https://github.com/encourotacy/RM_2.git (fetch)
origin  https://github.com/encourotacy/RM_2.git (push)
```

### 2.3 用 Cursor 打开

1. 打开 Cursor
2. **File → Open Folder**（打开文件夹）
3. 选中刚克隆出来的 `RM_2` 文件夹（选仓库根目录，不要只选 `class_3`）
4. 之后改 `class_3/main.cpp`、`Class3.md` 等都是在编辑 **这份 Git 仓库**

这和 Cursor 的 **Connect via SSH** 不是一回事。Connect via SSH 是去登录 **另一台服务器** 写那里的文件。协作同一 GitHub 仓库，用 `clone` + 打开文件夹即可，不必 SSH 进别人电脑。

### 2.4 以后每次开机改代码

```bash
cd RM_2 所在路径
git pull                    # 先拉别人或另一台电脑已经 push 的内容
# 在 Cursor 里改文件并保存
git add -A
git status                  # 看将要提交什么
git commit -m "说明这次改了什么"
git push
```

另一台电脑要看到更新：在那台机器的 `RM_2` 里再 `git pull`。

---

## 3. 日常命令（都在仓库根目录执行）

下面假设当前目录已经是 `RM_2`。

### 看状态

```bash
git status
git log --oneline -10
```

### 改完后提交并上传

```bash
git add -A
git commit -m "简短说明，例如：补充 Class3 作业说明"
git push origin main
```

已经设置过上游时，`git push` 即可。

### 下载 GitHub 上的最新代码

```bash
git pull origin main
```

或：

```bash
git pull
```

### 只想放弃某次还没 add 的修改

只对「还没 commit」的改动有效，提交过的不要用这条乱试：

```bash
git checkout -- 某个文件路径
```

### 拉别人的仓库（和 RM_2 分开）

另开文件夹 `git clone`，**不要**在本仓库里执行 `git remote set-url origin` 指向别人的库，否则 `push` 会推错地方。

```bash
cd ..
git clone https://github.com/某个用户/某个仓库.git
```

---

## 4. 国内访问 GitHub：代理（HTTPS）

Ubuntu 自带 Git 用 GnuTLS，直连 GitHub 时常见报错：

```text
fatal: 无法访问 'https://github.com/encourotacy/RM_2.git/'：
GnuTLS recv error (-110): The TLS connection was non-properly terminated.
```

这是网络/TLS 被掐断，不是命令写错。本仓库已在 `.git/config` 里写了（只对这个仓库生效）：

```text
[http]
	version = HTTP/1.1
[http "https://github.com"]
	proxy = http://127.0.0.1:7897
```

`7897` 是本机 Clash Verge 的 mixed-port。换电脑后：

1. 打开 Clash（或同类代理），看 **实际端口**（可能是 `7890` / `7897`）
2. 在**那台电脑**的仓库里设置（把端口改成你的）：

```bash
cd RM_2
git config http.version HTTP/1.1
git config http.https://github.com.proxy http://127.0.0.1:7897
```

若希望这台电脑上 **所有 GitHub 仓库** 都走代理，用 `--global`：

```bash
git config --global http.version HTTP/1.1
git config --global http.https://github.com.proxy http://127.0.0.1:7897
```

Clash 必须开着，且节点能打开 GitHub。Cursor 左侧「同步」失败时，改到 **终端** 里执行 `git pull` / `git push`，更容易看到是 TLS 还是要登录。

取消代理（不用时）：

```bash
git config --unset http.https://github.com.proxy
git config --global --unset http.https://github.com.proxy
```

---

## 5. 用户名、密码和 Token（亲测易错）

终端里 `git clone` / `git pull` / `git push` 若出现：

```text
Username for 'https://github.com':
Password for 'https://encourotacy@github.com':
```

填法如下（这是 **GitHub 给 Git 用的账号**，不是 Linux 用户 `star`）：

| 提示 | 填什么 | 不要填什么 |
|------|--------|------------|
| Username | `encourotacy`（仓库网址里那一段） | 电脑用户名 `star`、邮箱（除非 GitHub 明确要求） |
| Password | **Personal Access Token**（一长串，常以 `ghp_` 开头） | GitHub **网站登录密码** |

GitHub 从几年前起就 **禁止用登录密码做 Git 操作**。填登录密码时会出现：

```text
remote: Invalid username or token. Password authentication is not supported for Git operations.
fatal: 'https://github.com/encourotacy/RM_2.git/' 鉴权失败
```

这和 GnuTLS `-110` 不是一类问题：`-110` 是网络被掐；上面这句是 **已经连上 GitHub，但密码不对**。

### 5.1 生成 Token

1. 开着 Clash，浏览器打开 <https://github.com/settings/tokens>
2. **Generate new token** → **Generate new token (classic)**
3. Note 随便写，例如 `git-clone`；Expiration 选一个时长（到期要再生成）
4. 勾选 **`repo`**（整组勾上，私有库读写都靠它）
5. 拉到最底下 **Generate token**
6. 复制整串（只显示一次）。不要写进本 md，不要发到聊天里

Fine-grained token 也可以，但必须授权仓库 `RM_2` 且 Contents 为读写；新手用 classic + `repo` 更不容易漏勾。

### 5.2 克隆时怎么贴

```bash
git clone https://github.com/encourotacy/RM_2.git
```

- Username：`encourotacy`
- Password：终端里 **Ctrl+Shift+V** 粘贴 token。粘贴时屏幕往往 **完全不显示字符**，属正常，回车即可

若这次鉴权失败但已经出现半截 `RM_2` 文件夹，先删再克隆：

```bash
cd ~
rm -rf RM_2
git clone https://github.com/encourotacy/RM_2.git
```

仍提示 `Invalid username or token` 时检查：token 没复制全、多了空格、没勾 `repo`、Fine-grained 没勾选本仓库、填的还是登录密码。

第一次成功后，可让 Git 记住（明文存在家目录，注意电脑安全）：

```bash
git config --global credential.helper store
```

下次同机一般不用再贴 token。

---

## 6. 私有仓库为什么麻烦，改成公开能省掉什么

`RM_2` 目前是 **private**。没登录时 GitHub 会假装仓库不存在（网页/API 常返回 404），所以 `clone` / `pull` 也要 Token。这就是「换一台电脑还要输用户名密码」的主要原因。

| 操作 | 私有仓库 | 公开仓库（Public） |
|------|----------|-------------------|
| `git clone` / `git pull` | 必须 Username + Token（或 SSH） | 一般 **不用登录**，复制 HTTPS 地址就能拉 |
| `git push` 上传你的修改 | 要登录 | **同样要登录**（证明你是 `encourotacy`） |
| GnuTLS `-110`、要开代理 | 和公不公开无关 | 一样可能遇到 |

因此：

- 只想另一台电脑 **下载课件、少输 Token**：可以把仓库改成 Public，`clone`/`pull` 会简单很多。
- 还要从那台电脑 **`push` 回来**：公开也要 Token 或 SSH，推送省不掉。
- 课件、作业图若不怕别人看见再公开；有 token、密码、内部资料就继续私有。

若决定公开：GitHub 网页 → 本仓库 **Settings → General → Danger Zone → Change repository visibility → Public**。改之前确认仓库里没有密钥。

公开之后，另一台电脑可以：

```bash
git clone https://github.com/encourotacy/RM_2.git
```

不再询问 Password。以后 `git push` 仍会问 Token。

---

## 7. SSH 和 Cursor「Connect via SSH」（可选，容易混）

### 7.1 Git 使用 SSH 地址

把远端从 HTTPS 换成：

```bash
git remote set-url origin git@github.com:encourotacy/RM_2.git
```

仍是 **同一个仓库**，只是换了一种登录方式。需要本机生成 SSH 密钥，并把 `~/.ssh/id_ed25519.pub` 加到 GitHub → Settings → SSH keys。配好后 `push` 通常不用每次贴 Token，有时也比 GnuTLS 稳。

没配密钥前不要换；继续用第 4、5 节的 HTTPS 即可。SSH 也解决不了「不想登录就能 push」——服务器仍然要确认是你。

### 7.2 Cursor 的 Connect via SSH

那是 **Remote SSH**：Cursor 去控制另一台已开 SSH 的电脑，编辑那里的文件夹。  
别人邀请你进 GitHub 仓库协作，**不需要**这一步，`git clone` 后打开本地 `RM_2` 即可。

---

## 8. 别人邀请你一起协作时

对方在 GitHub 仓库 **Settings → Collaborators** 邀请你的账号。你接受后：

```bash
git clone https://github.com/对方用户名/对方仓库.git
```

用 Cursor **打开克隆下来的文件夹**。日常仍是 `pull` → 修改 → `commit` → `push`。  
不要把本仓库的 `origin` 改成对方地址。对方仓库是另一份 `clone`。

两人同时改同一文件再 `push`，可能产生冲突。先 `git pull` 再改，冲突时按提示打开文件，留下该留的内容后：

```bash
git add 冲突文件
git commit -m "合并冲突"
git push
```

---

## 9. 换电脑检查清单

在新电脑第一次工作前过一遍：

1. [ ] Clash（或代理）已开，GitHub 网页能打开
2. [ ] `git clone https://github.com/encourotacy/RM_2.git`
3. [ ] `git remote -v` 确认是你的 `RM_2`
4. [ ] 如有 GnuTLS `-110`，按第 4 节给这个仓库配 `http.proxy` 和 `HTTP/1.1`
5. [ ] Cursor → Open Folder → 打开 `RM_2` 根目录
6. [ ] 私有库：`clone`/`pull` 用 Username `encourotacy` + Token，不要填网站密码
7. [ ] 若已改成 Public，clone 可不登录；push 仍要 Token
8. [ ] 改一个小文件，`commit` + `push`，到网页上能看到

之后在这台电脑上就只重复：打开同一文件夹 → `pull` → 编辑 → `commit` → `push`。

---

## 10. 本仓库 `.git/config` 在干什么

根目录 `.git/config` 是 **这个仓库自己的 Git 设置**（换电脑 clone 后默认没有代理那几行，需要的话按第 4 节再加）。当前这台电脑上大致是：

- `remote.origin.url`：GitHub 上的 RM_2（HTTPS）
- `branch.main.remote`：跟踪 `origin` 的 `main`
- `http.https://github.com.proxy`：走本机 Clash `7897`
- `http.version`：`HTTP/1.1`，减轻 GnuTLS 问题

改代码、提交历史不在这个文件里；不要把 token 写进 config。

---

## 11. 出问题对照

| 现象 | 常见原因 | 怎么处理 |
|------|----------|----------|
| GnuTLS recv error (-110) | 没走代理 / 节点不行 / Cursor 同步按钮不稳定 | 开 Clash，确认端口，终端里 `git pull`；配第 4 节 proxy |
| Password authentication is not supported / Invalid username or token | Password 填了 GitHub **登录密码**，或 Token 无效、没勾 `repo` | 见第 5 节，Password 处贴 `ghp_` Token |
| 要 Username / 没有那个设备或地址 | 私有库未登录，或非交互环境无法输入 | 终端里 clone/pull/push，用户名 `encourotacy` + Token |
| 私有库 clone 很烦、只想下载 | 仓库是 private | 见第 6 节；仅下载可改 Public，push 仍要登录 |
| `origin` 指向别人的库 | 误执行了 `set-url` | `git remote set-url origin https://github.com/encourotacy/RM_2.git` |
| 另一台电脑没有你刚改的内容 | 只保存或只 commit，没 push；或那边没 pull | 这边 `git push`，那边 `git pull` |
| Cursor Connect via SSH 连不上队友 | 那是远程桌面式开发，不是 Git | 让对方加你为 Collaborator，你自己 clone |

本地文件不会因为一次 `pull`/`push` 失败而消失。连上并登录成功后再执行同一条命令即可。
