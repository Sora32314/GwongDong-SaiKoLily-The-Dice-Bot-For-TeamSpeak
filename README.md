# 骰娘小广东 / GwongDong SaiKoLily

GwongDong SaiKoLily（骰娘小广东）是运行于 TeamSpeak 3 客户端的 TRPG 掷骰插件。用户可在频道聊天框输入骰子表达式进行投掷，并通过会话记录成员与投掷历史；会话记录可导出为文本文件。

当前插件内部版本为 `0.0.3`（内测）。一期功能已完成，公开测试 v0.4 待测试日结束后发布。

## 功能概览

- 掷骰：支持 `XdY` 形式的加减表达式与常数项，例如 `2d6+1d4-3`、`d20+5`。
- 会话：支持创建、选择、结束会话，以及加入、移除成员和设置管理员。
- 历史：可查看当前会话的最近投掷记录。
- 导出：可将当前会话的投掷记录导出为文本文件。
- 帮助：`.ss help` 可查看会话子命令的语法、权限和示例；`.ss help start` 查看跑团快速开始。
- 身份：成员身份优先按 TeamSpeak 3 UUID 记录，重连或改名不会丢失。

## 构建

### 环境要求

- Windows
- CMake 4.0.1 或更高版本
- Ninja
- MinGW-w64 GCC 14 或更高版本（开发环境使用 GCC 16.1 验证）
- C++23

spdlog 1.15.3 与 TeamSpeak 3 插件 SDK v26 已包含在 `ExternModules/` 中，无需额外安装。

### 构建命令

```powershell
# Debug
cmake --preset "core debug"
cmake --build --preset "Debug Build"

# Release
cmake --preset "core release"
cmake --build --preset "Release Build"
```

产物位于 `Build/Debug/bin` 或 `Build/Release/bin`。MinGW 生成的 DLL 为 `libPlugin_GwongDong.dll`，MSVC 生成 `Plugin_GwongDong.dll`。MinGW 采用静态运行时链接，部署时只需复制 DLL。

手动指定编译器：

```powershell
cmake -S . -B Build/Release -G Ninja `
  -DCMAKE_BUILD_TYPE=Release `
  -DCMAKE_CXX_COMPILER=g++
cmake --build Build/Release --target Plugin_GwongDong
```

## 安装到 TeamSpeak 3

1. 将 `libPlugin_GwongDong.dll`（MSVC 构建为 `Plugin_GwongDong.dll`）复制到 TeamSpeak 3 客户端插件目录。Windows 默认路径通常为 `%APPDATA%\TS3Client\plugins`。
2. 重启客户端，在「工具 → 选项 → 插件」中确认插件已加载。
3. 进入频道后，在聊天框输入命令。

插件启动时会通过 `guiConnect` 连接 `localhost`，因此本机需运行一个 TeamSpeak 3 服务器。连接使用的 Identity 固定写在 `Modules/Plugin_GwongDong/src/Init_Plugin.cpp` 的 `ts3plugin_init()` 中；如需连接其他服务器，应修改配置并重新编译。

插件会忽略自身发送的消息；不以命令前缀开头的普通聊天不会触发命令处理。

## 快速开始

1. 创建会话：`.ss create 黑水溪 "团本：黑水溪是本次跑团的模组"`
2. 加入玩家：`.ss Join 玩家A 玩家B`
3. 开始掷骰：`!r 2d6+1d4-3`
4. 查看与导出：`.ss history 10`、`.ss STF ./logs 跑团记录 txt`

> **重要**：随机数引擎需要在加入会话后使用。未加入会话时单独掷骰会使用临时引擎，结果可能出现较大偏差，仅适合演示。

在聊天框执行 `.ss help start` 可以查看完整示例。

## 命令说明

### 通用规则

- 命令前缀为 `!`、`.` 或 `>`，前缀后紧跟命令名。
- 参数按空白字符拆分；包含空格时需使用单引号或双引号包裹。
- 命令名与别名按精确字符串匹配，区分大小写。
- `.ss` 支持在一条消息中串联多个子命令；`select` 会优先执行。
- 当前会话按“用户选择 → 已加入 → 频道绑定”的顺序确定。
- `info` 在未选中会话时会回退到以当前用户为创建者的会话；其他依赖工作会话的命令会失败。
- 用户参数按当前服务器在线昵称精确匹配，不支持离线查询。
- 会话查找方式：`id` / `index` / `i`、`title` / `t`、`creator`。

权限等级：

| 等级 | 说明 |
| --- | --- |
| `Creator` | 会话创建者 |
| `Admins` | 会话管理员 |
| `Member` | 会话成员 |
| `NonMember` | 非会话成员 |
| `Banned` | 预留等级 |

当前没有工作会话时，用户等级按 `NonMember` 处理；需要 `Member`、`Admins` 或 `Creator` 权限的命令会返回权限不足。

### 掷骰

```text
<前缀><roll|r|rd|dice|throw> <expression>
```

`expression` 为必填参数，必须作为单个参数传入；包含空格时需加引号。命令只读取第一个参数，后续参数会被忽略。

| 参数 | 必需 | 说明 |
| --- | --- | --- |
| `expression` | 是 | 骰子表达式 |

表达式语法：

```text
expression := [sign] term { ("+" | "-") term }
term       := [count] "d" faces | integer
```

- `count`：骰子数量，省略时为 1，例如 `d6` 等价于 `1d6`。
- `faces`：骰子面数。
- `integer`：常数项，可与骰子项混合。
- 表达式内部允许空格；空格会在解析时忽略。

示例：

```text
!r 1d20
!r 2d6+1d4-3
!r 1d20+5-1d4
!r "2d6 + 1d4 - 3"
```

输出：

```text
投掷结果：2d6[3, 5]+1d4[2]-3
点数：7
```

> **重要**：随机数引擎需要在加入会话后使用。未加入会话时单独掷骰会使用临时引擎，结果可能出现较大偏差，仅适合演示。

任意用户均可执行。存在当前工作会话时，投掷记录会写入该会话历史；否则使用临时骰子系统，不写入历史。首个表达式项不显示前导符号，建议以正数项开头。

### 会话

```text
<前缀><ss|session|sessions|game> <子命令> [参数...]
```

| 子命令 | 语法 | 权限 | 说明 |
| --- | --- | --- | --- |
| `create` | `.ss create <title> [description]` | 任意用户 | 创建会话 |
| `select` | `.ss select <method> <value>` | 须为成员 | 选定当前会话 |
| `unselect` | `.ss unselect [<method> <value>]` | 任意用户 | 取消选择 |
| `end` | `.ss end [<method> <value>]` | 管理员 | 结束会话 |
| `Join` | `.ss Join <user> [user...]` | 成员 | 加入成员 |
| `Quit` | `.ss Quit <user> [user...]` | 管理员 | 移除成员 |
| `info` | `.ss info` | 任意用户 | 查看会话信息 |
| `list` | `.ss list` | 任意用户 | 列出会话 |
| `search` | `.ss search <method> <value>` | 任意用户 | 搜索会话 |
| `history` | `.ss history [N]` | 成员 | 查看投掷历史，默认 5 条，最多 50 条 |
| `STF` | `.ss STF [directory] [filename] [extension]` | 创建者 | 导出投掷记录 |
| `adminset` | `.ss adminset <user> [user...]` | 创建者 | 添加管理员 |
| `adminsremove` | `.ss adminsremove <user> [user...]` | 创建者 | 移除管理员 |
| `adminslist` | `.ss adminslist` | 成员 | 查看管理员列表 |
| `help` | `.ss help [subcommand]` | 任意用户 | 显示会话命令帮助 |

常用别名：`create` / `new`、`end` / `stop`、`select` / `选择`、`history` / `历史`、`STF` / `保存`、`help` / `帮助`。`Join`、`Quit` 使用首字母大写形式；全小写 `join`、`quit` 未注册。

公共参数：

| 参数 | 说明 |
| --- | --- |
| `<method>` | `id` / `index` / `i`、`title` / `t`、`creator` |
| `<value>` | 会话 ID、精确标题或精确创建者昵称 |
| `<title>` | 会话标题；含空格时加引号 |
| `[description]` | 会话描述；含空格时加引号 |
| `<user>` | 当前服务器在线昵称，精确匹配；可传多个 |
| `[N]` | `.ss history`：1–50，默认 5；`.ch list`：1–100，默认 20 |
| `[directory]` | 导出目录，默认 `./logs` |
| `[filename]` | 文件名，默认会话标题 |
| `[extension]` | 扩展名，默认 `txt` |

示例：

```text
.ss create 黑水溪 "团本：黑水溪是本次跑团的模组"
.ss select title 黑水溪
.ss Join Sora32314 某人
.ss history 10
.ss STF ./logs 跑团记录 txt
.ss help history
.ss help start
```

`.ss help` 不带参数时列出会话子命令；`.ss help <子命令>` 显示该子命令的语法、权限、别名和示例；`.ss help start` 查看跑团快速开始。

占位或尚未实现的子命令：`status`（仅写日志）、`activate`、`rest`、`userlist`、`bind`、`cleanhistory`、`set`、`export`、`import`。

### 顶层 checkhistory

```text
<前缀><checkhistory|ch|his|history|check> list [N]
```

- `list [N]`：按时间倒序显示最近 N 条记录，默认 20 条，上限 100 条。
- `check` / `clear` 目前只返回固定字符串；`detail`、`stats`、`last`、`ID`、`Name` 未实现。
- 日常查询建议使用 `.ss history`。

## 日志与数据

- 日志写入 `logs/async_logs.txt`，使用 spdlog 异步文件日志；相对路径基于 TeamSpeak 3 客户端进程工作目录。
- 会话、成员、管理员与投掷记录均保存在内存中，插件重启后清空。需要留档时使用 `.ss STF` 导出。
- 会话 ID 为运行时分配，重启后从 1 重新开始。
- 导出文件由 `GwongDongFileSystems` 写入，正文前包含自定义 MetaData 头，不是纯正文文本。
- 导出目录不存在时会自动创建；重复导出到同一路径会覆盖原文件。
- 每条记录包含玩家昵称、投掷时间、带出目的表达式、最终点数和备注。

## 项目结构

```text
Modules/
├─ CommandCore/           命令注册、分发与上下文
├─ Loggings/             spdlog 异步日志封装
├─ GwongDongFileSystems/  带 MetaData 的文件读写
├─ SaiKoLily/             骰子、会话与命令实现
└─ Plugin_GwongDong/      TeamSpeak 3 插件入口，生成 DLL
ExternModules/
├─ spdlog-1.15.3/
└─ ts3client-pluginsdk-26/
```

依赖关系：

```text
command_core           -> encapsulation_spdlog
gwongdong_filesystem   -> encapsulation_spdlog
saiko_lily             -> command_core / gwongdong_filesystem / encapsulation_spdlog
Plugin_GwongDong       -> saiko_lily / command_core / gwongdong_filesystem / encapsulation_spdlog
```

`DiceSystem` 负责骰子与投掷事件，`SessionManager` 负责会话、成员与命令处理；命令实现位于 `CMD_SaiKoLily.hpp` 与 `SessionManager.hpp`。

## 已知限制

- 当前仅支持 Windows 与 TeamSpeak 3 客户端（Plugin API v26）。
- 连接地址与 Identity 在当前测试版本为了便于调试直接固定写在代码中，如需连接其他服务器，需要修改配置并重新编译或进行手动连接。
- 会话数据未持久化，插件重启后将丢失。
- 用户查询依赖当前服务器在线客户端缓存，仅支持昵称精确匹配，不支持离线用户。
- 子命令别名区分大小写；`Join`、`Quit` 等需使用已注册的大小写形式。
- `status`、`activate`、`rest` 等子命令尚未完成；`userlist` 等命令尚未实现。
- 顶层 `checkhistory` 仅 `list` 可用。
- 掷骰输出对首个表达式项不显示前导符号；建议以正数项开头。
- 导出文件包含自定义 MetaData 头，外部工具读取时需要跳过头部。

## 许可证

本项目采用 MIT 许可证，详见 [LICENSE](LICENSE)。

作者：Sora32314