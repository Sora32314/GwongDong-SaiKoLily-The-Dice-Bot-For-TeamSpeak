#pragma once
#include <deque>
#include <random>
#include <string_view>
#include <CommandCore.hpp>

using TimePoint = std::chrono::system_clock::time_point;

namespace SaiKoLily
{
    namespace DiceSystem
    {
        class Dice;
        class DiceConfig;
        class DiceEventImpl;

        /**
         * @brief 骰子投掷事件抽象接口
         *
         * `IDiceEvent` 封装了一次或多次骰子投掷（可能包含多个骰子项、修正值）的完整信息，
         * 包括投掷结果、投掷者标识、时间戳以及构成此次投掷的原始配置项。
         *
         * 该接口将骰子事件的“数据”与“行为”解耦：派生类负责存储具体结果与配置，
         * 并通过接口方法暴露只读访问。同时提供有限的修改接口（如设置修正值、添加骰子项），
         * 供表达式解析器在构建过程中填充。
         *
         * 主要使用场景：
         * - 作为 `IDiceSystem` 投掷方法的返回值包装类型；
         * - 存储于 `IDiceSystem` 的全局投掷历史队列与玩家投掷历史映射中；
         * - 通过 `GetTermDetails()` 提供用于日志格式化或复盘展示的详细结果。
         *
         * @note 该接口禁止拷贝，允许移动，确保骰子事件对象的所有权唯一且可转移。
         *       实际对象由骰子系统工厂创建，通过 `std::shared_ptr<IDiceEvent>` 共享管理。
         *
         * @see IDiceSystem, DiceConfig
         */
        class IDiceEvent
        {
        public:
            IDiceEvent() = default;
            virtual ~IDiceEvent() = default;

            /// 禁止拷贝
            IDiceEvent(const IDiceEvent&) = delete;
            IDiceEvent& operator=(const IDiceEvent&) = delete;

            /// 允许移动
            IDiceEvent(IDiceEvent&&) = default;
            IDiceEvent& operator=(IDiceEvent&&) = default;

        public:
            /**
             * @brief 执行实际的骰子投掷逻辑
             *
             * 调用此方法将根据当前存储的骰子配置项和修正值生成随机结果。
             * 该方法通常由 `IDiceSystem` 在构建事件后调用，或在延迟求值场景下触发。
             */
            virtual void RollDiceEvent() = 0;

            /**
             * @brief 获取每个单独骰子的原始出目结果列表
             * @return 包含每个骰子投掷结果的常量引用向量（如 [3, 5, 1]）
             */
            virtual const std::vector<int64_t>& GetResultsList() = 0;

            /**
             * @brief 获取每个投掷项（term）的点数和列表
             *
             * 对于一个骰子表达式 `2d6 + 1d4 + 3`，此方法返回三个值：
             * 第一项（2d6）的总和、第二项（1d4）的总和、第三项（固定值 3）。
             *
             * @return 各投掷项的点数和的向量
             */
            virtual std::vector<int64_t> GetPointsList() = 0;

            /**
             * @brief 获取此次事件涉及的骰子总个数
             * @return 所有骰子项中骰子数量的累加值
             */
            virtual uint32_t GetDiceCount() = 0;

            /**
             * @brief 获取所有骰子的面数配置列表
             *
             * 对于 `2d6 + 1d20`，可能返回 [6, 6, 20]（具体顺序取决于实现）。
             *
             * @return 骰子面数向量
             */
            virtual std::vector<int64_t> GetFaces() = 0;

            /**
             * @brief 获取投掷此事件的玩家标识符
             * @return 玩家 ID 字符串（通常为用户昵称或 UUID）
             */
            virtual std::string GetPlayerID() = 0;

            /**
             * @brief 获取事件发生的时间点
             * @return 系统时钟时间点
             */
            virtual TimePoint GetTimeInfo() = 0;

            /**
             * @brief 获取本次投掷的总点数（含修正值）
             * @return 所有骰子结果之和加上修正偏移量
             */
            virtual int64_t GetPointsInTotal() = 0;

            /**
             * @brief 获取当前事件的修正偏移量
             * @return 加在骰子总和上的固定修正值（可正可负）
             */
            virtual int64_t GetOffset() = 0;

            /**
             * @brief 获取构成此次事件的骰子项原始数据（深层结构）
             *
             * 返回一个二维向量，外层每个元素代表表达式中的一个投掷项（如 `2d6` 为一个项），
             * 内层向量包含该投掷项中每个骰子的 `Dice` 对象描述（面数、可能的重投规则等）。
             *
             * @return 骰子项数据的常量引用
             */
            virtual const std::vector<std::vector<Dice>>& GetDiceTerms() const = 0;

            /**
             * @brief 获取压缩后的骰子配置项列表
             *
             * 将骰子项按配置分组，返回每个 `DiceConfig` 及其重复次数的配对列表。
             * 例如 `2d6 + 1d6` 可能被合并为 `{DiceConfig(6), 3}`。
             *
             * @return 配置与计数的配对向量常量引用
             */
            virtual const std::vector<std::pair<const DiceConfig, uint32_t>>& GetConfigTerms() const = 0;

            /**
             * @brief 获取此次投掷的格式化日志字符串
             *
             * 该字符串通常包含表达式原文、每个骰子的出目、总和以及修正值信息，
             * 用于向用户展示详细投掷过程。
             *
             * @return 投掷日志字符串
             */
            virtual const std::string GetThrowLog() const = 0;

            // ---------- 设置方法（构建阶段使用） ----------
            /**
             * @brief 设置修正偏移量
             * @param _offset 新的修正值
             */
            virtual void SetOffset(int64_t _offset) = 0;

            /**
             * @brief 设置完整的骰子项数据（移动语义）
             * @param _dice_terms 二维骰子对象向量，将接管所有权
             */
            virtual void SetDiceTerms(std::vector<std::vector<Dice>>&& _dice_terms) = 0;

            /**
             * @brief 添加一个投掷项
             * @param _dice_terms 表示一个投掷项的骰子向量
             */
            virtual void AddDiceTerms(std::vector<Dice>&& _dice_terms) = 0;

            /**
             * @brief 设置投掷日志内容
             * @param log 日志字符串视图
             */
            virtual void SetThrowLog(std::string_view log) = 0;

            /**
             * @brief 解析并生成表达式的可读字符串表示
             *
             * 根据内部存储的 `DiceTerms` 或 `ConfigTerms` 反向生成如 "2d6+1d4+3" 的字符串。
             *
             * @return 表达式字符串
             */
            virtual std::string ParseExpressionStr() = 0;

            /**
             * @brief 获取每个投掷项内每个骰子的详细出目
             *
             * 返回二维向量，外层对应每个投掷项，内层为该投掷项内各骰子的具体点数。
             * 此方法主要用于生成详细的日志输出。
             *
             * @return 二维点数向量的常量引用
             */
            virtual std::vector<std::vector<int64_t>> GetTermDetails() const = 0;
        };

        /**
         * @brief 骰子投掷系统核心接口
         *
         * `IDiceSystem` 是 SaiKoLily 插件中处理一切骰子投掷、历史记录管理与随机数生成的中心接口。
         * 它提供了多种重载的 `RollDice` 方法以支持从简单的单一面数投掷到复杂多表达式组合，
         * 并维护全局投掷顺序历史与按玩家分组的投掷记录。
         *
         * 一个 `IDiceSystem` 实例通常与一个会话（Session）或一个长期上下文（如频道）绑定，
         * 内部持有独立的随机数生成器（`std::mt19937_64`）和种子序列，确保投掷的独立性与可重现性。
         *
         * 主要功能分组：
         * - **投掷操作**：`RollDice` 系列方法，执行投掷并返回总点数，同时将事件存入历史。
         * - **结果查询**：`GetResult` / `GetLastResult` / `GetThrow` / `GetLastEvent` 获取最近一次投掷信息。
         * - **历史访问**：提供对全局投掷队列和按玩家分组历史的只读/可读写访问。
         * - **随机数引擎访问**：暴露内部 RNG，供需要自定义随机逻辑的模块使用。
         *
         * @note 该接口禁止拷贝，允许移动，确保会话骰子系统的唯一所有权。
         *       具体实现通过 `IDiceSystemFactory` 工厂创建。
         *
         * @see IDiceEvent, DiceConfig, IDiceSystemFactory
         */
        class IDiceSystem
        {
        public:
            virtual ~IDiceSystem() = default;
            IDiceSystem() = default;

            /// 禁止拷贝与赋值
            IDiceSystem(const IDiceSystem&) = delete;
            IDiceSystem& operator=(const IDiceSystem&) = delete;

            /// 允许移动
            IDiceSystem(IDiceSystem&&) = default;
            IDiceSystem& operator=(IDiceSystem&&) = default;

            /**
             * @brief 投掷单个自由面数骰子
             *
             * 根据给定的面数投掷一次，结果存储于内部并可通过 `GetResult()` 获取。
             *
             * @param face 骰子面数（引用传递，但通常不修改）
             * @param player 投掷者标识，默认为 "Unknow"
             * @return 投掷结果（1 到 face 之间的整数）
             */
            virtual int64_t RollDice(int64_t& face, std::string_view player = "Unknow") = 0;

            /**
             * @brief 根据配置项列表执行复杂投掷
             *
             * 接受一组骰子配置及其数量的配对，加上一个固定修正值，执行一次复合投掷。
             * 内部会构造 `IDiceEvent` 对象，执行随机数生成，记录历史，并返回总点数。
             *
             * @param configs 骰子配置与计数的配对列表（例如 `{DiceConfig(6), 2}` 表示 2d6）
             * @param player 投掷者标识
             * @param offset 总点数修正值
             * @return 投掷总点数（含修正）
             */
            virtual int64_t RollDice(std::vector<std::pair<DiceConfig, uint32_t>>& configs, std::string_view player = "Unknow", int64_t offset = 0) = 0;

            /**
             * @brief 获取最近一次投掷的总点数结果
             * @return 总点数
             */
            virtual int64_t GetResult() = 0;

            /**
             * @brief 获取上一次投掷的总点数结果（同 `GetResult`）
             * @return 总点数
             */
            virtual int64_t GetLastResult() = 0;

            /**
             * @brief 获取最近一次投掷的事件对象指针
             * @return 指向最新 `IDiceEvent` 的原始指针，若无历史则返回 `nullptr`
             */
            virtual IDiceEvent* GetThrow() = 0;

            /**
             * @brief 获取最近一次投掷的事件对象指针（同 `GetThrow`）
             * @return 事件指针
             */
            virtual IDiceEvent* GetLastEvent() = 0;

            /**
             * @brief 获取全局投掷事件队列（只读）
             *
             * 队列按投掷发生的时间顺序存储所有事件，最新的事件位于队列尾部。
             *
             * @return 包含 `shared_ptr<IDiceEvent>` 的双端队列常量引用
             */
            virtual const std::deque<std::shared_ptr<IDiceEvent>>& GetDiceEventListConst() = 0;

            /**
             * @brief 按玩家 ID 获取该玩家的历史投掷事件列表（只读）
             *
             * @param playerID 玩家标识符
             * @return 该玩家所有事件智能指针向量的常量引用
             */
            virtual const std::vector<std::shared_ptr<IDiceEvent>>& GetDiceEventListByNameConst(std::string_view playerID) = 0;

            /**
             * @brief 获取按玩家 ID 分组的完整历史记录映射（只读）
             *
             * 映射的键为玩家 ID，值为该玩家所有投掷事件的智能指针向量。
             *
             * @return 映射的常量引用
             */
            virtual const std::map<std::string, std::vector<std::shared_ptr<IDiceEvent>>>& GetPlayerDiceList() const = 0;

            /**
             * @brief 设置指定玩家的历史投掷列表（危险操作）
             *
             * 用移动语义替换某玩家的全部历史记录。应谨慎使用，通常用于状态恢复或测试。
             *
             * @param playerID 目标玩家 ID
             * @param player_dices_lists 新的历史事件向量（将移动赋值）
             */
            virtual void SetGlobalPlayerDiceList(std::string playerID, std::vector<std::shared_ptr<IDiceEvent>>&& player_dices_lists) = 0;

            /**
             * @brief 获取指定玩家的历史投掷事件列表（可修改）
             *
             * 返回引用，允许调用者直接添加或修改该玩家的历史记录。
             *
             * @param playerID 玩家标识符
             * @return 该玩家事件向量的引用
             */
            virtual std::vector<std::shared_ptr<IDiceEvent>>& GetPlayerDiceListByID(std::string_view playerID) = 0;

            /**
             * @brief 获取全局投掷事件队列（只读）
             * @return 全局队列的常量引用
             */
            virtual const std::deque<std::shared_ptr<IDiceEvent>>& GetGlobalDiceList() const = 0;

            /**
             * @brief 设置全局投掷事件队列（危险操作）
             *
             * 替换整个全局历史队列。用于状态加载或重置。
             *
             * @param global_dices_lists 新的全局事件队列（将移动赋值）
             */
            virtual void SetGlobalDiceList(std::deque<std::shared_ptr<IDiceEvent>>&& global_dices_lists) = 0;

            /**
             * @brief 获取全局投掷事件队列（可修改）
             *
             * 返回引用，允许直接操作全局历史（如清空、手动添加）。
             *
             * @return 全局队列的引用
             */
            virtual std::deque<std::shared_ptr<IDiceEvent>>& GetGlobalDiceList() = 0;

            /**
             * @brief 获取内部随机数生成器引擎
             * @return `std::mt19937_64` 引擎的引用
             */
            virtual std::mt19937_64& GetRNG() = 0;

            /**
             * @brief 获取用于初始化 RNG 的种子序列
             * @return 种子序列的引用
             */
            virtual std::seed_seq& GetSeedSeq() = 0;
        };

        /**
         * @brief 骰子系统抽象工厂接口
         *
         * `IDiceSystemFactory` 是用于创建 `IDiceSystem` 实例的抽象工厂。
         * 采用工厂模式将骰子系统的具体实现与使用方解耦，使得 SaiKoLily 核心
         * 可以通过全局函数 `GetDiceSystemFactory()` 获取注册的工厂，并调用
         * `CreateDiceSystem()` 生成新实例。
         *
         * 典型用法：
         * - 插件加载时，骰子系统实现模块通过 `SetDiceSystemFactory()` 注册其工厂。
         * - 会话管理器在创建新会话时，调用 `factory->CreateDiceSystem()` 获取
         *   专属的骰子系统实例，并与会话绑定。
         *
         * @see IDiceSystem, SetDiceSystemFactory, GetDiceSystemFactory
         */
        class IDiceSystemFactory
        {
        public:
            virtual ~IDiceSystemFactory() = default;

            /**
             * @brief 创建一个新的骰子系统实例
             * @return 指向新创建的 `IDiceSystem` 对象的独占指针
             */
            virtual std::unique_ptr<SaiKoLily::DiceSystem::IDiceSystem> CreateDiceSystem() = 0;
        };

        /**
         * @brief 获取当前全局注册的骰子系统工厂
         *
         * 返回由 `SetDiceSystemFactory` 设置的工厂指针。该指针由插件初始化代码管理生命周期，
         * 调用方不应负责释放。
         *
         * @return 指向 `IDiceSystemFactory` 的原始指针，若未设置则返回 `nullptr`
         *
         * @see SetDiceSystemFactory
         */
        IDiceSystemFactory* GetDiceSystemFactory();

        /**
         * @brief 设置全局骰子系统工厂
         *
         * 插件初始化时调用此函数注册具体的工厂实现。传入的指针所指向的对象
         * 应在插件整个生命周期内保持有效。
         *
         * @param factory 工厂实现类的指针，通常为单例或静态对象
         *
         * @see GetDiceSystemFactory
         */
        void SetDiceSystemFactory(IDiceSystemFactory* factory);
    }

    /**
     * @brief 骰子上下文提供者接口
     *
     * `IDiceContextProvider` 定义了一种根据命令执行上下文获取对应 `IDiceSystem` 实例的策略。
     * 在 SaiKoLily 的架构中，骰子系统可能与特定的会话（Session）或频道绑定，因此不同的
     * 命令上下文（例如来自不同群聊或私聊的用户）需要映射到不同的骰子系统实例。
     *
     * 该接口将“如何获取骰子系统”这一决策抽象出来，由外部（通常是会话管理模块）实现并注册。
     * 命令处理器在需要执行投掷时，通过全局函数 `GetDiceContextProvider()` 获取此提供者，
     * 并调用 `GetDiceSystem(context)` 获得当前上下文所关联的骰子系统。
     *
     * 典型实现：
     * - 根据 `context.GetServerID()` 和 `context.GetChannelID()` 查找对应的会话对象。
     * - 从会话对象中提取其持有的 `IDiceSystem` 唯一指针并返回原始指针。
     *
     * @see SetDiceContextProvider, GetDiceContextProvider, Command_Core::ICommandContext
     */
    class IDiceContextProvider
    {
    public:
        virtual ~IDiceContextProvider() = default;

        /**
         * @brief 根据给定的命令上下文获取对应的骰子系统实例
         *
         * @param context 命令执行上下文，包含用户、频道、服务器等环境信息
         * @return 指向与上下文关联的 `IDiceSystem` 对象的原始指针，若无关联则返回 `nullptr`
         */
        virtual SaiKoLily::DiceSystem::IDiceSystem* GetDiceSystem(const Command_Core::ICommandContext& context) = 0;
    };

    /**
     * @brief 设置全局骰子上下文提供者
     *
     * 通常在会话管理器初始化时调用，将自身作为提供者注册到 SaiKoLily 核心。
     *
     * @param provider 指向 `IDiceContextProvider` 实现的指针，生命周期由调用方管理
     *
     * @see IDiceContextProvider, GetDiceContextProvider
     */
    void SetDiceContextProvider(IDiceContextProvider* provider);

    /**
     * @brief 获取当前全局骰子上下文提供者
     *
     * @return 指向 `IDiceContextProvider` 的原始指针，若未设置则返回 `nullptr`
     *
     * @see SetDiceContextProvider
     */
    IDiceContextProvider* GetDiceContextProvider();
}

namespace Sessions::SessionTemp
{
    /**
     * @brief 会话抽象接口（临时命名空间）
     *
     * `ISession` 代表一个独立的交互上下文，例如一个跑团房间、一个多人游戏会话。
     * 该接口目前处于 `SessionTemp` 命名空间下，表明其为过渡性设计，未来可能迁移至正式模块。
     *
     * 会话对象管理其专属的资源，其中最重要的是 `IDiceSystem` 实例。
     * 每个会话拥有独立的骰子历史、随机数生成器，从而实现环境隔离。
     *
     * @note 该接口禁止拷贝，确保会话资源的唯一所有权。
     *
     * @see Sessions::SessionManagerTemp::ISessionManager
     */
    class ISession
    {
    public:
        virtual ~ISession() = default;
        ISession() = default;

        /// 禁止拷贝
        ISession(const ISession&) = delete;
        ISession& operator=(const ISession&) = delete;

        /**
         * @brief 获取此会话绑定的骰子系统实例
         *
         * 返回会话内部持有的 `IDiceSystem` 独占指针的引用，允许调用者通过此引用
         * 访问骰子系统的所有功能。
         *
         * @return 指向 `IDiceSystem` 的独占指针的引用
         */
        virtual std::unique_ptr<SaiKoLily::DiceSystem::IDiceSystem>& GetDiceSystem() = 0;
    };
}

namespace Sessions
{
    /// 会话获取方式枚举（前向声明，具体定义在其他头文件）
    enum class SessionFetchMethod;
}

namespace Sessions::SessionManagerTemp
{
    /**
     * @brief 会话管理器抽象接口（临时命名空间）
     *
     * `ISessionManager` 负责管理所有活动会话的生命周期，并提供基于不同策略的会话获取方法。
     * 它是连接用户命令与具体会话对象的桥梁，同时也是 `IDiceContextProvider` 的天然实现者。
     *
     * 主要功能：
     * - **会话获取**：通过 `GetSession` 方法，传入获取方式（如按 ID、按名称）和参数，
     *   返回对应的会话对象指针。
     * - **用户选择状态管理**：维护一个从用户 UUID 到当前选中会话 ID 的映射，
     *   用于支持用户在不指定会话 ID 的情况下，默认操作其“当前选中”的会话。
     *
     * @note 该接口位于 `SessionManagerTemp` 命名空间，表明其设计可能随会话模块重构而变化。
     *       允许移动构造/赋值，以支持工厂返回或容器存储。
     *
     * @see ISession, SessionFetchMethod
     */
    class ISessionManager
    {
    public:
        virtual ~ISessionManager() = default;
        ISessionManager() = default;

        /// 禁止拷贝
        ISessionManager(const ISessionManager&) = delete;
        ISessionManager& operator=(const ISessionManager&) = delete;

        /// 允许移动
        ISessionManager(ISessionManager&&) = default;
        ISessionManager& operator=(ISessionManager&&) = default;

        /**
         * @brief 根据指定的方法和参数获取会话
         *
         * 获取方式由 `SessionFetchMethod` 枚举定义，例如：
         * - 按会话 ID 查找
         * - 按会话名称查找
         * - 获取用户当前选中的会话
         *
         * @param method 会话获取方法枚举值
         * @param arg 方法所需的参数（如 ID 字符串或名称）
         * @return 若找到则返回指向 `ISession` 的指针包装在 `std::optional` 中，否则返回 `std::nullopt`
         */
        virtual std::optional<Sessions::SessionTemp::ISession*> GetSession(Sessions::SessionFetchMethod& method, std::string_view arg) = 0;

        /**
         * @brief 获取用户 UUID 到当前选中会话 ID 的映射表（只读）
         *
         * 该映射记录了每个用户当前“激活”的会话，用于支持 `SessionFetchMethod::Selected` 查询。
         *
         * @return 映射的常量引用，键为用户 UUID，值为会话 ID
         */
        virtual const std::unordered_map<std::string, ID>& GetSelectionOfSession() const = 0;
    };
}