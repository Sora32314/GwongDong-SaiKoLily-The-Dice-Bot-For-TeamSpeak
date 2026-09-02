# GwongDong-SaiKoLily（骰娘小广东）

运行在 TeamSpeak 3 客户端内的掷骰 Bot，面向 TRPG 玩家。可以在语音频道里输入骰子表达式即时掷骰，通过「会话」管理跑团成员、记录每一次投掷，并把历史导出为文本文件。

[![version](https://img.shields.io/badge/version-v0.0.3--insider-blue)](https://github.com/Sora32314/GwongDong-SaiKoLily)
[![C++](https://img.shields.io/badge/C%2B%2B-23-%23f34b7d)](https://en.cppreference.com/w/cpp/23)
[![TeamSpeak](https://img.shields.io/badge/TeamSpeak%203%20Plugin%20API-v26-orange)](https://www.teamspeak.com/)
[![toolchain](https://img.shields.io/badge/toolchain-MinGW--w64%20GCC%2014%2B-brightgreen)](https://www.mingw-w64.org/)
[![license](https://img.shields.io/badge/license-MIT-green)](LICENSE)

---

## 目录

- [GwongDong-SaiKoLily（骰娘小广东）](#gwongdong-saikolily骰娘小广东)
  - [目录](#目录)
  - [简介](#简介)
  - [功能一览](#功能一览)
  - [ 📦 环境要求与构建](#--环境要求与构建)
    - [环境要求](#环境要求)
    - [使用 CMake Presets 构建](#使用-cmake-presets-构建)
    - [使用 MinGW-w64 GCC 构建](#使用-mingw-w64-gcc-构建)
  - [ 🚀 安装到 TeamSpeak 3](#--安装到-teamspeak-3)
  - [ ⌨️ 命令参考](#-️-命令参考)
    - [通用规则](#通用规则)
    - [ 🎲 掷骰命令](#--掷骰命令)
    - [ 🗂️ 会话命令](#-️-会话命令)
    - [ 📜 历史记录与导出](#--历史记录与导出)
  - [ ⚙️ 配置与日志](#-️-配置与日志)
  - [ 🏗️ 项目结构](#-️-项目结构)
  - [许可证](#许可证)
    - [The Project Made With ❤~](#the-project-made-with-)
    - [作者：Sora32314](#作者sora32314)

---

## 简介

插件以「会话（Session）」为单位组织跑团活动。一个会话代表一次跑团，包含标题、描述、成员列表以及独立的一份投掷历史：

- 会话外的临时掷骰只做演示，结果不会保留；
- 创建或加入会话后，掷骰会进入该会话的历史，供查询与导出。

## 功能一览

- **掷骰**：解析 `XdY` 形式的多项加减表达式（如 `2d6+1d4-3`）并返回每个骰子的出目与总点数。
- **会话管理**：创建 / 选择 / 结束会话，向会话中加入或移除成员。
- **历史记录**：按时间倒序查看最近若干次投掷。
- **历史导出**：把会话内全部投掷记录写入文本文件。

## <a id="build"></a> 📦 环境要求与构建

### 环境要求

| 依赖 | 说明 |
| --- | --- |
| 操作系统 | Windows |
| TeamSpeak 3 客户端 | 支持 Plugin API v26 |
| 编译器 | MinGW-w64 GCC 14+ |
| C++ 标准 | C++23 |
| CMake | ≥ 4.0.1 |
| Ninja | 构建生成器 |
| spdlog | 1.15.3，仓库内 `ExternModules/spdlog-1.15.3`，无需单独安装 |

### 使用 CMake Presets 构建

仓库提供了 [`CMakePresets.json`](CMakePresets.json:1)，预设中为开发便利将编译器写死为 MSVC `cl.exe`。若你的 `PATH` 中没有 MSVC 环境（`vcvars64.bat`），请改用下面的 MinGW 手动配置方式。

```bash
# Debug
cmake --preset "core debug"
cmake --build --preset "Debug Build"
```

### 使用 MinGW-w64 GCC 构建

```bash
# 配置（Release 示例，需覆盖预设中默认的 MSVC 编译器）
cmake -S . -B Build/Release -G Ninja ^
  -DCMAKE_BUILD_TYPE=Release ^
  -DCMAKE_C_COMPILER=gcc ^
  -DCMAKE_CXX_COMPILER=g++

# 编译插件
cmake --build Build/Release --target Plugin_GwongDong
```

编译产物（`.dll`）输出到 `Build/Release/bin`（Debug 对应 `Build/Debug/bin`）。

## <a id="install"></a> 🚀 安装到 TeamSpeak 3

1. 把编译出的 `Plugin_GwongDong.dll`（其依赖的静态库已链入，无需额外文件）复制到 TeamSpeak 3 客户端的插件目录。
2. 启动 TeamSpeak 3 客户端，在「工具 → 选项 → 插件」中确认插件已加载。
3. 插件加载时会尝试通过 `guiConnect` 连接到本地 `localhost` 服务器（连接信息硬编码于 [`Init_Plugin.cpp`](Modules/Plugin_GwongDong/src/Init_Plugin.cpp:350)，可按需修改后重新编译）。
4. 进入某个频道后即可在聊天框输入命令。插件会自动订阅频道、缓存成员列表，用于把投掷结果对应到具体玩家。

## <a id="commands"></a> ⌨️ 命令参考

### 通用规则

- 所有命令以 `!`、`.`、`>` 之一开头，后接命令名与参数。
- 命令与别名注册表同时收录了大小写变体，匹配按精确字符串进行；顶层命令支持别名（见各小节）。
- 含空格的参数请用 `"` 或 `'` 包裹。
- 只有以命令前缀开头的内容才会被当作命令处理，普通聊天不会触发。

### <a id="roll"></a> 🎲 掷骰命令

| 命令 | 别名 | 示例 |
| --- | --- | --- |
| `roll` | `r`、`rd`、`dice`、`throw` | `!r 2d6+1d4-3`、`.rd 1d20+5`、`>throw 3d10` |

**表达式语法**

- `XdY`：投掷 X 个 Y 面骰，例如 `2d6`；省略数量时默认掷 1 个（`d6` 等价于 `1d6`）。
- 支持 `+` / `-` 拼接多个骰子项：`2d6+1d4-3`。
- 支持纯常数项：`3` 直接返回 3，也可与骰子混合，如 `1d20+5-1d4`。
- 表达式内空格会被忽略。

**输出示例**

输入：

```
!r 2d6+1d4-3
```

频道内输出：

```
投掷结果：2d6[3, 5]+1d4[2]-3
点数：7
```

> `[3, 5]` 是 2 个 d6 各自掷出的出目；`点数` 为全部出目与常数项之和。

### <a id="sessions"></a> 🗂️ 会话命令

会话命令为 `ss`（也可写作 `sessions`、`session`、`game`），格式：

```
.ss <子命令> [参数…]
```

子命令支持中英文别名，下表列出主要别名。多数操作作用于你的「当前会话」（见下方「会话命中优先级」），建议操作前先用 `.ss select` 选定目标会话。

**已实现**

| 子命令 | 别名 | 说明 | 示例 |
| --- | --- | --- | --- |
| `create` | `new`、`创建` | 创建会话；第 1 个参数为标题，第 2 个可选为描述 | `.ss create 黑水溪 团本：<黑水溪>是本次跑团的团本，是一个非常经典的模组。` |
| `end` | `stop`、`结束` | 结束会话。无参数时结束当前选中的会话；也可带 `<查找方式> <参数>` | `.ss end`、`.ss end title 黑水溪` |
| `select` | `选择` | 把指定会话设为当前选中会话 | `.ss select id 1`、`.ss select title 黑水溪` |
| `unselect` | `取消选择` | 取消当前选中的会话 | `.ss unselect` |
| `join` | `add`、`添加用户` | 把一个或多个用户加入当前会话 | `.ss join SenSei❤ 旱獭优香` |
| `quit` | `delete`、`删除用户` | 把一个或多个用户移出当前会话 | `.ss quit 生盐诺亚` |
| `info` | `信息` | 查看当前会话详情（ID、标题、描述、类型、创建者、成员） | `.ss info` |
| `list` | `ls`、`会话列表` | 列出当前所有活跃会话 | `.ss list` |
| `search` | `查询` | 按 `<查找方式> <参数>` 搜索会话 | `.ss search title 黑水溪` |
| `history` | `历史` | 查看当前会话最近 N 次投掷（默认 20 条） | `.ss history 10` |
| `STF` | `save`、`保存` | 把当前会话全部投掷历史导出为文件；参数依次为路径、文件名、扩展名（均可省略） | `.ss STF ./logs 跑团记录 txt` |

**查找方式**：`id` / `index`（按会话 ID）、`title`（按标题）、`creator`（按创建者）。

**会话命中优先级**：当前会话按以下顺序确定——

1. 用户主动 `select` 的会话；
2. 用户加入的会话（取最后加入的一个）；
3. 频道绑定的会话。

均未命中时，部分操作会回退到「以你为创建者的会话」。当前为内部版本，命令行为可能随开发调整。

其余子命令（`status`、`activate`、`rest`、`adminset`、`adminsremove`、`adminslist`、`userlist`、`bind`、`cleanhistory`、`set`、`export`、`import`、`help`）在命令表中已注册但尚未实现，调用不会生效。

### <a id="history"></a> 📜 历史记录与导出

- 会话内历史用 `.ss history [N]` 查看，默认显示最近 20 条，格式类似：

```
Session: 黑水溪 的历史记录获取成功:
Sora32314 在 2026年09月02日14时30分15秒 投掷出 2d6+1d4-3
```

- 导出用 `.ss STF`，默认生成在 `./logs`，文件以会话标题命名；也可通过参数指定目录、文件名与扩展名。
- 顶层另有一个独立历史查询命令 `history`（别名 `ch`、`his`、`check`），与掷骰命令同级，用于查询当前上下文的投掷历史；该模块在源码注释中被标注为「可能未被使用」。

## <a id="config"></a> ⚙️ 配置与日志

- **日志文件**：输出到 `logs/async_logs.txt`（基于 spdlog 的异步文件日志，见 [`Init_Plugin.cpp`](Modules/Plugin_GwongDong/src/Init_Plugin.cpp:22)）。日志包含命令分发、会话操作、骰子投掷等各模块的运行信息。
- **服务器连接**：插件加载时自动连接 `localhost`。需要修改目标服务器地址 / 身份等信息时，编辑 [`Init_Plugin.cpp`](Modules/Plugin_GwongDong/src/Init_Plugin.cpp:350) 中 `ts3plugin_init()` 里的 `guiConnect` 参数后重新编译。
- 外部配置项（历史条数上限、默认保存格式等）尚未开放，相关逻辑在代码中标注为待开发。

## <a id="structure"></a> 🏗️ 项目结构

工程采用多模块 CMake 组织。`ExternModules/` 为仓库内置的三方依赖：

```
ExternModules/
├─ spdlog-1.15.3/            # spdlog 源码（日志底层）
└─ ts3client-pluginsdk-26/   # TeamSpeak 3 客户端插件 SDK v26
Modules/
├─ CommandCore/              # command_core：命令注册 / 分发 / 上下文
├─ Loggings/                 # encapsulation_spdlog：spdlog 异步封装
├─ GwongDongFileSystems/     # gwongdong_filesystem：带元数据的文件读写，用于历史导出
├─ SaiKoLily/                # saiko_lily：骰子系统 / 会话管理 / 命令实现（核心）
├─ Plugin_GwongDong/         # Plugin_GwongDong：TS3 插件入口，DLL 目标
├─ TS3FuncWrapper/           # ts3wrapper：TS3 API 轻封装（独立共享库）
└─ HelperFuncs/              # 辅助函数（暂未接入构建）
```

各模块（静态库）依赖关系：

```
静态库 command_core          依赖 encapsulation_spdlog
静态库 gwongdong_filesystem   依赖 encapsulation_spdlog
静态库 saiko_lily            依赖 command_core / gwongdong_filesystem / encapsulation_spdlog
共享库 Plugin_GwongDong      依赖 saiko_lily / command_core / gwongdong_filesystem / encapsulation_spdlog，输出最终插件 DLL
共享库 ts3wrapper            TeamSpeak 3 API 轻封装，独立目标，为未来预留拓展空间。
```

- [`SaiKoLily.hpp`](Modules/SaiKoLily/include/SaiKoLily.hpp:41) 提供骰子系统（`DiceSystem`）的骰子/配置/投掷事件实现；
- [`SessionManager.hpp`](Modules/SaiKoLily/include/SessionManager.hpp:27) 提供会话生命周期与用户管理，每个会话绑定独立骰子系统；
- 会话模块的类图设计见 [`MAP/SessionManager.puml`](MAP/SessionManager.puml:1)（PlantUML）。

## 许可证

本项目基于 MIT 协议开源，详见 [LICENSE](LICENSE) 文件。

### The Project Made With ❤~

### 作者：Sora32314
