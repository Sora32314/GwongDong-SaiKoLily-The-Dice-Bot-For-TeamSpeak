#pragma once

#include <map>
#include <vector>
#include <memory>
#include <optional>
#include <functional>
#include <string_view>
#include <loggings.hpp>


using uint64 = unsigned long long;
using ID = uint64_t;


/**
 * @brief 这个命名空间用于存放命令处理相关的内容。
 * 
 * 这个命名空间提供了命令处理相关的接口和实现。
 * 命令处理相关的内容包括命令注册、命令执行、命令检查等。
 * 命令处理相关的接口和实现都位于这个命名空间中。
 * 命名空间中的内容被设计为与命令处理相关的内容。
 * 
 */
namespace Command_Core
{
    using anyID = unsigned short;

    /**
     * @brief 此结构体用来描述如何获取用户的信息。
     * 
     * userID: 用户ID
     * channelID: 频道ID
     * serverID: 服务器ID
     * serverUUID: 服务器UUID
     * UserName: 用户名
     * ChannelName: 频道名
     * ServerName: 服务器名
     */
    struct InfoFetcher
    {
        uint64 userID;
        uint64 channelID;
        uint64 serverID;
        std::string serverUUID;
        std::optional<std::string> UserName;
        std::optional<std::string> ChannelName;
        std::optional<std::string> ServerName;
    };

    /**
     * @brief ClientInfoPackage 结构体用来描述用户信息。
     * 
     * 客户端信息类型包
     * 不存储ServerConnectionHandler，只要她们UUID一致，在逻辑上认为她们是同一个。
     * 
     * nickName: 用户名
     * clientID: 用户ID
     * channelID: 频道ID
     * uniqueID: UUID
     */
    typedef struct {
        std::string nickName;
        anyID clientID;
        uint64_t channelID;
        std::string uniqueID;
    } ClientInfoPackage;

    using CC_LogCallback = std::function<void(const std::string&, Plugin_Logs::logLevel, bool nowFlush)>;
    void SetLogCallback(CC_LogCallback callback);


    /**
     * @brief 会话参与者抽象标识结构体
     *
     * User 在 `SessionManager` 模块中作为统一的用户身份标识，用于会话成员管理、用户选择状态跟踪、
     * 重复加入检测以及命令执行时的临时构造。它不依赖具体聊天平台的原生用户对象，仅通过
     * userID、uuid 和 name 抽象跨平台身份。
     *
     * 主要使用场景：
     * - 作为 `SessionImpl::users` 容器的元素，记录会话内普通参与者；
     * - 作为 `AddUser()`、`RemoveUser()` 的批量操作参数；
     * - 通过 `UserSelectSession()` / `UserUnselectSession()` 建立 UUID 到 SessionID 的映射；
     * - 在命令处理器中由 `ClientInfoPackage` 临时构造，作为操作目标。
     *
     * 字段 channelID 和 serverID 为 optional，允许在不同会话作用域下灵活填充；
     * 结构体以值类型传递，避免智能指针开销。
     *
     * @note Admin 继承自 User，额外增加 PermissionLevel 字段，二者共享成员管理接口的底层容器。
     */
    struct User
    {
        User(std::string_view name = "Unknow", uint64 channelID = 0, uint64 serverID = 0, ID userID = 0, std::string uuid = "Unknow")
        : name(name),
        channelID(channelID),
        serverID(serverID),
        userID(userID),
        uuid(uuid)
        {}

        std::string name;
        std::optional<uint64> channelID;
        std::optional<uint64> serverID;
        ID userID;
        std::string uuid;
    };
    

    /**
     * @brief 管理员结构体
     * 
     * 此结构体继承至User，额外增加了PermissionLevel字段，用于标识会话管理员权限等级，二者共享管理接口的底层容器。
     * 
     * @warning 此结构体暂未被使用！
     * 
     * @author Sora32314
     * @date 2026-4-16 21:06
     */
    struct Admin : public User
    {
        int PermissionLevel = 0;
    };

    
    /**
     * @brief 信息来源目标
     * 
     * 此枚举用于标识信息的来源目标，作用是用来确定信息即将被转发的目标。 
     * 
     * @note
     * 信息来源目标枚举，用于标识信息的来源目标。
     * 当信息的来源目为私信时，会话管理模块会执行将信息转发给用户。
     * 当信息的来源目标为频道时，会话管理模块会执行将信息转发给频道。
     * 当信息的来源目标为服务器时，会话管理模块会执行将信息转发给服务器。
     * 
     * 
     */
    enum class MessageTarget
    {
        /// @brief 1：私信
        PrivateMessage = 1,
        /// @brief 2：当前频道
        CurrentChannel,
        /// @brief 3：当前服务器
        Server
    };

    
    /**
     * @brief 结果发送回调接口
     *
     * 定义向指定目标投递消息的统一方法。命令框架通过此接口将执行结果返回给用户，
     * 而无需关心底层通信协议。外部调用者实现 `SendResult` 以适配具体平台（如 WebSocket、HTTP API），
     * 从而实现消息发送逻辑与命令核心的解耦。
     *
     * 典型用法：外部调用者创建实现类并注入到 `ICommandChecker::SetResultCallback()` 中，
     * 命令检查器在处理完消息后调用 `SendResult` 返回响应。
     *
     * @note 同一个命令框架可通过注入不同的 `IResultCallback` 实现来适配不同前端环境，
     *       无需修改命令处理逻辑。
     * @see ICommandChecker::SetResultCallback, ICommandContext::SendResult
     */   
    class IResultCallback
    {
    public:
        virtual ~IResultCallback() = default;

        /**
         * @brief 向指定目标发送消息
         * 
         * @param userID 用户ID
         * @param channelID 频道ID
         * @param serverID 服务器ID
         * @param target 目标类型
         * @param message 消息内容
         */
        virtual void SendResult
        (
            uint64 userID,
            uint64 channelID,
            uint64 serverID,
            MessageTarget target,
            const std::string& message
        ) = 0;
    };

    /**
     * @brief 命令执行上下文接口
     * 
     * `ICommandContext` 是 `CommandCore` 的核心接口之一，作为命令执行所需所有上下文信息的聚合载体，
     * 也是命令处理器与外部系统交互的唯一门面。它封装了当前命令的原始文本、调用者身份、所在频道/服务器、
     * 命令参数、日志记录、结果发送以及在线用户查询等全部功能。
     * 
     * 实现该接口的类由命令框架（如 `ICommandChecker`）在每次处理消息时创建并注入到命令处理器中。
     * 通过 `ICommandContext`，命令处理器可以完全独立于底层通信细节，只需关注业务逻辑的实现。
     * 
     * @note
     * - 该接口内部通常持有一个 `InfoFetcher` 实例，其 `Get...()` 方法直接暴露 `InfoFetcher` 中的数据。
     * - `SendResult()` 的实现通常会转发调用到预先注册的 `IResultCallback` 对象上，完成实际消息投递。
     * 
     * @see Command_Core::InfoFetcher, Command_Core::IResultCallback
     */
    class ICommandContext
    {
    public:
        virtual ~ICommandContext() = default;

    public:
        /**
         * @brief 获取触发当前命令的原始消息文本（包含命令前缀和参数）
         * @return 原始命令字符串视图，生命周期与上下文对象相同
         */
        virtual std::string_view GetRawCommand() const = 0;

        /**
         * @brief 获取命令调用者的昵称/用户名
         * @return 调用者名称的字符串视图
         */
        virtual std::string_view GetCallingUserName() const = 0;

        /**
         * @brief 获取当前命令所在频道的名称
         * @return 频道名称的字符串视图，若为私聊则返回空或占位字符串
         */
        virtual std::string_view GetChannelName() const = 0;

        /**
         * @brief 获取当前命令所在服务器的名称
         * @return 服务器名称的字符串视图，若为私聊或不存在则返回空或占位字符串
         */
        virtual std::string_view GetServerName() const = 0;

        /**
         * @brief 获取命令调用者的唯一数字标识符
         * @return 用户 ID
         */
        virtual uint64 GetCallingUserID() const = 0;

        /**
         * @brief 获取当前命令所在频道的唯一数字标识符
         * @return 频道 ID，私聊场景可能返回 0 或特殊值
         */
        virtual uint64 GetChannelID() const = 0;

        /**
         * @brief 获取当前命令所在服务器的唯一数字标识符
         * @return 服务器 ID，私聊或频道场景可能返回 0 或特殊值
         */
        virtual uint64 GetServerID() const = 0;

        /**
         * @brief 记录日志信息
         * @param message 需要记录的消息内容
         * @param level 日志等级（info / warn / err 等）
         * @param nowFlush 是否立即刷新到持久化存储或输出设备
         */
        virtual void Log(std::string_view message, Plugin_Logs::logLevel level, bool nowFlush) const = 0;

        /**
         * @brief 获取按空格分割后的命令参数
         * @param index 参数索引，从 0 开始（0 为第一个参数，不包含命令本身）
         * @return 若索引有效则返回对应参数字符串视图，否则返回 std::nullopt
         */
        virtual std::optional<std::string_view> GetParam(size_t index) const = 0;

        /**
         * @brief 获取命令参数的总个数
         * @return 参数个数
         */
        virtual size_t GetParamCount() const = 0;

        /**
         * @brief 向用户发送命令执行结果
         * @param message 待发送的消息内容
         * @param target 消息投递目标，默认为当前频道（CurrentChannel）
         */
        virtual void SendResult(const std::string& message, MessageTarget target = MessageTarget::CurrentChannel) const = 0;

        /**
         * @brief 获取当前消息的目标类型（私聊 / 当前频道 / 服务器广播）
         * @return MessageTarget 枚举值
         */
        virtual MessageTarget GetMessageTarget() const = 0;

        /**
         * @brief 获取当前频道内所有在线用户的信息
         * @return 以用户 UUID 为键、ClientInfoPackage 为值的无序映射
         */
        virtual std::unordered_map<std::string, ClientInfoPackage> GetAllUsersInChannel() const = 0;

        /**
         * @brief 获取当前服务器内所有在线用户的信息
         * @return 以用户 UUID 为键、ClientInfoPackage 为值的无序映射
         */
        virtual std::unordered_map<std::string, ClientInfoPackage> GetAllUsersInServer() const = 0;

        /**
         * @brief 获取所有已知在线用户的信息（跨服务器/频道）
         * @return 以用户 UUID 为键、ClientInfoPackage 为值的无序映射
         */
        virtual std::unordered_map<std::string, ClientInfoPackage> GetAllUsers() const = 0;

        /**
         * @brief 通过用户 ID 获取其昵称列表
         * @param userID 目标用户的唯一标识符
         * @return 匹配到的用户昵称集合（同一用户可能在多端登录或历史数据导致多条记录）
         */
        virtual std::vector<std::string> GetUserNameByID(uint64 userID) const = 0;

        /**
         * @brief 通过服务器句柄和用户 ID 获取该服务器内指定用户的昵称列表
         * @param serverHandler 服务器连接句柄
         * @param userID 用户 ID
         * @return 昵称列表
         */
        virtual std::vector<std::string> GetUserNameByID(uint64_t serverHandler, uint64 userID) const = 0;

        /**
         * @brief 获取指定用户的全局唯一标识符（UUID）
         * @param userID 用户 ID
         * @return UUID 字符串视图
         */
        virtual std::string_view GetUserUUID(uint64 userID) const = 0;

        /**
         * @brief 通过用户 ID 获取其完整的客户端信息包
         * @param userID 用户 ID
         * @return ClientInfoPackage 结构体，包含昵称、频道 ID、UUID 等字段
         */
        virtual ClientInfoPackage GetUser(uint64 userID) const = 0;

        /**
         * @brief 通过昵称获取用户信息（昵称在服务器范围内唯一）
         * @param name 用户昵称
         * @return 匹配到的 ClientInfoPackage，若未找到则返回空的 uniqueID 字段
         */
        virtual ClientInfoPackage GetUserByName(std::string_view name) const = 0;
    };

    /**
     * @brief 命令处理器抽象接口
     *
     * `ICommandHandler` 是 `CommandCore` 框架中定义具体命令行为的核心接口。
     * 每一个可被用户触发的命令（如 `.roll`、`.session create`）都对应一个该接口的实现类。
     * 框架通过命令注册器（`ICommandRegistry`）管理所有处理器实例，并在用户输入匹配的命令名
     * 或别名时，调用对应的 `Execute` 方法执行业务逻辑。
     *
     * 实现该接口的类需要提供以下信息：
     * - 命令的正式名称（用于主标识）
     * - 命令的功能描述（用于自动生成帮助信息）
     * - 一组命令别名（用于多触发词支持）
     *
     * 当命令被触发时，框架会将当前执行环境的上下文（`ICommandContext`）以引用的方式传入
     * `Execute` 方法。处理器可通过该上下文获取参数、发送回复、记录日志以及查询在线用户，
     * 而无需关心底层消息通信细节。
     *
     * @note 处理器实例通过 `ICommandRegistry::RegisterHandler` 以独占所有权的方式注册，
     *       框架会在销毁时自动释放资源。一个命令处理器应专注于单一命令逻辑，
     *       多个命令应分别实现多个处理器。
     *
     * @see ICommandRegistry, ICommandContext, ICommandChecker
     */
    class ICommandHandler
    {
    public:
        /**
         * @brief 执行命令的具体业务逻辑
         *
         * 当用户输入匹配当前命令（通过 `GetName()` 或 `GetAlias()` 匹配）时，
         * 框架会构造一个包含环境信息和用户输入的 `ICommandContext` 对象，
         * 并调用此方法。实现者应在此方法内完成命令的核心功能。
         *
         * @param context 命令执行上下文，提供参数解析、用户查询、结果发送等能力
         */
        virtual void Execute(ICommandContext& context) = 0;

        /**
         * @brief 获取命令的正式名称
         *
         * 该名称作为命令的主要标识符，用于框架内部映射和帮助系统的显示。
         * 应返回唯一且稳定的字符串视图，通常使用 PascalCase 或全小写单词。
         *
         * @return 命令名称的字符串视图
         */
        virtual std::string_view GetName() const = 0;

        /**
         * @brief 获取命令的功能描述
         *
         * 用于自动生成帮助文本或向用户展示命令用途。描述应简洁明了，
         * 说明命令的基本功能和用法。可包含参数占位符说明，如 `"<表达式> [备注]"`。
         *
         * @return 命令描述的字符串视图
         */
        virtual std::string_view GetDescription() const = 0;

        /**
         * @brief 获取命令的别名列表
         *
         * 返回一组可触发此命令的替代名称。框架在匹配用户输入时会同时检查正式名称和所有别名。
         * 别名不区分大小写（具体行为由匹配逻辑决定），允许同一个处理器响应多个用户习惯的写法。
         *
         * @return 别名视图的向量，若无需别名可返回空向量
         */
        virtual std::vector<std::string_view> GetAlias() const = 0;

        virtual ~ICommandHandler() = default;
    };

    /**
     * @brief 命令注册器接口
     *
     * `ICommandRegistry` 是 `CommandCore` 框架中管理所有命令处理器的注册表。
     * 它负责存储 `ICommandHandler` 实例并建立命令名（含别名）到处理器的映射关系，
     * 使得命令检查器（`ICommandChecker`）能够根据用户输入的指令快速定位对应的处理器。
     *
     * 该接口遵循单一职责原则，仅提供注册与查找两种操作：
     * - `RegisterHandler`：以独占所有权的方式接纳命令处理器。
     * - `FindHandler`：根据给定的命令字符串（正式名或别名）查询对应的处理器指针。
     *
     * 外部调用者（通常为插件初始化代码）通过工厂函数 `CreateRegistry()` 获取该接口的
     * 具体实现实例，并调用 `RegisterHandler` 逐个注册自定义命令。框架在处理每条消息时，
     * 会使用 `FindHandler` 确定要调用的处理器，从而实现命令的动态分发。
     *
     * @note 注册器持有处理器的所有权，当注册器析构时所有已注册的命令处理器会被自动销毁。
     *       `FindHandler` 返回的指针仅用于临时调用，不应被调用方缓存或释放。
     *
     * @see ICommandHandler, ICommandChecker, Command_Core::CreateRegistry()
     */
    class ICommandRegistry
    {
    public:
        virtual ~ICommandRegistry() = default;

        /**
         * @brief 注册一个命令处理器
         *
         * 将传入的命令处理器实例纳入注册表管理，并建立其正式名称和所有别名到该处理器的映射。
         * 如果存在名称冲突（已注册相同名称或别名的处理器），实现类的行为未定义（建议覆盖或抛出异常）。
         *
         * @param handler 命令处理器的独占指针，调用后所有权转移至注册器
         */
        virtual void RegisterHandler(std::unique_ptr<ICommandHandler> handler) = 0;

        /**
         * @brief 根据命令字符串查找对应的处理器
         *
         * 在已注册的处理器集合中搜索与给定字符串匹配的命令。匹配规则为：
         * 先比对正式名称，若匹配失败则遍历所有别名。匹配通常不区分大小写（具体由实现决定）。
         *
         * @param command 待查找的命令字符串（通常为用户输入的第一个词，如 `.ss`、`roll`）
         * @return 若找到匹配的处理器，返回其原始指针；否则返回 `nullptr`
         */
        virtual ICommandHandler* FindHandler(std::string_view command) const = 0;
    };

    /**
     * @brief 命令检查与分发接口
     *
     * `ICommandChecker` 是 `CommandCore` 框架中负责接收原始消息、解析命令前缀、定位匹配的
     * 命令处理器并触发执行的协调者。它处于消息输入与业务逻辑之间的核心调度位置。
     *
     * 典型工作流程：
     * 1. 外部系统（如聊天平台适配层）获取用户消息后，构造 `InfoFetcher` 描述环境信息。
     * 2. 调用 `ProcessMessage` 将原始文本、环境信息和消息目标传入。
     * 3. `ICommandChecker` 内部执行命令前缀识别（如检测 `.` 或 `!`）、参数切割，并通过
     *    `ICommandRegistry` 查找匹配的 `ICommandHandler`。
     * 4. 若找到处理器，则构造 `ICommandContext` 并调用 `handler->Execute()`；
     *    若未找到或执行过程中需返回结果，则通过预先设置的 `IResultCallback` 将响应投递给用户。
     *
     * 该接口将命令解析、路由、上下文构建和结果回调等模板化流程封装起来，使得外部调用者
     * 仅需关注消息的来源和目标，而无需重复实现命令分发的底层机制。
     *
     * @note 必须先通过 `SetResultCallback` 设置结果回调，否则命令执行结果将无法返回给用户。
     *       工厂函数 `CreateCMDChecker` 用于创建该接口的具体实现。
     *
     * @see ICommandRegistry, ICommandHandler, IResultCallback, InfoFetcher
     */
    class ICommandChecker
    {
    public:
        virtual ~ICommandChecker() = default;

        /**
         * @brief 设置结果发送回调
         *
         * 注入一个 `IResultCallback` 实现，用于在命令执行过程中或执行完毕后
         * 将消息投递给用户。该回调由外部调用者提供，负责与具体通信后端交互。
         *
         * @param callback 结果回调接口的共享指针，内部通常保存其弱引用或共享所有权
         */
        virtual void SetResultCallback(std::shared_ptr<IResultCallback> callback) = 0;

        /**
         * @brief 处理一条原始消息，尝试解析并执行对应命令
         *
         * 接收用户输入的原始文本、环境信息以及消息的目标类型，完成命令的识别、
         * 解析、分发与执行。如果文本不符合命令格式或未找到匹配处理器，可选择
         * 通过结果回调返回提示信息或静默忽略（具体行为由实现决定）。
         *
         * @param message 用户发送的原始消息字符串，可能包含命令前缀和参数
         * @param info_package 环境信息包，包含用户 ID、频道 ID、服务器 UUID 等
         * @param messageTarget 消息的投递目标类型（私聊 / 当前频道 / 服务器广播）
         * @return 如果消息被识别为有效命令并成功进入处理流程，返回 `true`；
         *         否则返回 `false`（例如非命令文本或处理器未找到）
         */
        virtual bool ProcessMessage(
            std::string_view message,
            InfoFetcher info_package,
            MessageTarget messageTarget
        ) = 0;
    };

    /**
     * @brief 客户端信息缓存访问接口
     *
     * `IClientInfoStorage` 提供对当前已连接客户端信息缓存的直接访问能力。
     * 该接口封装了按服务器分组存储的用户信息表，允许其他模块（如 `SessionManager` 中的
     * `UserManagerImpl`）高效地查询在线用户的昵称、UUID 和频道归属等数据。
     *
     * 缓存结构为两层映射：
     * - 外层键：服务器连接句柄（`uint64_t`），标识不同的服务器实例；
     * - 内层键：用户 UUID（`std::string`）；
     * - 内层值：`ClientInfoPackage` 结构体，包含昵称、客户端 ID、频道 ID 等信息。
     *
     * 该接口由命令框架的核心实现提供，并通过全局函数 `Command_Core::GetClientInfoStorage()`
     * 暴露单例引用。调用者通过 `GetClientCache()` 获取缓存映射的可读写引用，以便进行
     * 遍历、查找或更新操作。
     *
     * @note 该接口暴露了内部数据结构的直接引用，调用方需自行保证线程安全。
     *       `SessionManager` 中的 `UserManagerImpl` 使用 `std::shared_mutex` 来保护并发访问。
     *
     * @see ClientInfoPackage, Command_Core::GetClientInfoStorage()
     */
    class IClientInfoStorage
    {
    public:
        virtual ~IClientInfoStorage() = default;

        /**
         * @brief 获取客户端信息缓存映射的引用
         *
         * 返回一个两层无序映射的引用，外层键为服务器连接句柄，内层键为用户 UUID，
         * 值为对应的 `ClientInfoPackage` 结构。调用方可通过此引用读取或修改缓存的用户信息。
         *
         * @return 缓存的非 const 引用
         */
        virtual std::unordered_map<uint64_t, std::unordered_map<std::string, ClientInfoPackage>>& GetClientCache() = 0;
    };

    //获取客户端信息缓存接口
    IClientInfoStorage& GetClientInfoStorage();

    /**
     * @brief 创建命令注册器实例
     *
     * 工厂函数，用于生成 `ICommandRegistry` 接口的默认实现对象。
     * 返回的注册器以独占所有权方式持有所有已注册的命令处理器。
     *
     * @return 指向新创建的 `ICommandRegistry` 实例的独占指针
     *
     * @see ICommandRegistry, CreateCMDChecker
     */
    std::unique_ptr<ICommandRegistry> CreateRegistry();

    /**
     * @brief 创建命令检查器实例
     *
     * 工厂函数，用于生成 `ICommandChecker` 接口的默认实现对象。
     * 创建的检查器依赖于给定的命令注册器，用于在消息处理过程中查找匹配的命令处理器。
     *
     * @param registry 命令注册器的引用，检查器将通过此注册器查找命令处理器
     * @return 指向新创建的 `ICommandChecker` 实例的独占指针
     *
     * @note 调用方需保证 `registry` 的生命周期长于创建的 `ICommandChecker` 对象，
     *       否则在检查器使用期间访问已销毁的注册器将导致未定义行为。
     *
     * @see ICommandChecker, ICommandRegistry, CreateRegistry
     */
    std::unique_ptr<ICommandChecker> CreateCMDChecker(ICommandRegistry& registry);

}



