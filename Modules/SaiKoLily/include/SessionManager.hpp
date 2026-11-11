#pragma once 

#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <charconv>
#include <functional>
#include <loggings.hpp>
#include <shared_mutex>
#include <CommandCore.hpp>
#include <GwongDongFileSystem.hpp>
#include <SaiKoLilyInterface.hpp>

using int64_t = long long;
using uint64 = unsigned long long;


// Debug 日志统一由 _DEBUG 控制（见 Modules/CMakeLists.txt）。

namespace SaiKoLily {
    namespace DiceSystem {
        class IDiceSystem;
    }
}
using IDiceSystem = SaiKoLily::DiceSystem::IDiceSystem;

namespace Sessions
{
    /**
     * @brief 会话模块日志回调类型定义
     *
     * 回调函数用于将会话管理模块内部日志输出到外部日志系统。
     *
     * @param message 日志消息
     * @param level 日志等级
     * @param nowFlush 是否立即刷新
     */
    using Sessions_LogCallback = std::function<void(const std::string&, Plugin_Logs::logLevel, bool nowFlush)>;

    /**
     * @brief 将投掷事件中的玩家标识解析为可显示的用户名。
     *
     * 优先使用投掷时的昵称快照（GetPlayerName），因此离线、换 UUID、身份重置后
     * 历史记录仍可读；快照为空时回退为原始玩家标识（UUID）。
     *
     * @param event 投掷事件
     * @return 可显示的用户名；查不到时回退为原始玩家标识
     */
    inline std::string ResolvePlayerName(auto& event)
    {
        const std::string snapshot = event.GetPlayerName();
        if(!snapshot.empty())
            return snapshot;

        const std::string playerID = event.GetPlayerID();
        return playerID.empty() ? std::string("Unknow") : playerID;
    }
    
    /**
     * @brief 设置会话模块的全局日志回调
     *
     * @param callback 日志回调函数
     */
    void SetLogCallback(Sessions_LogCallback callback);

    /**
     * @brief 会话获取方式枚举
     *
     * 定义根据何种属性来查找或获取一个会话对象。
     */
    extern enum class SessionFetchMethod
    {
        Title = 1,   ///< 按会话标题查找
        Creator,     ///< 按创建者名称查找
        Index        ///< 按会话ID（索引）查找
    } GetSessionBy;

    /**
     * @brief 会话列表过滤方式枚举
     *
     * 定义列出会话列表时依据的属性过滤条件。
     */
    extern enum class SessionListFilter
    {
        Creator = 1, ///< 按创建者过滤
        Server,      ///< 按所在服务器过滤
        Channel,     ///< 按所在频道过滤
        Auto         ///< 自动选择过滤方式
    } GetSessionListBy;

    /**
     * @brief 会话类型枚举
     *
     * 描述会话的可见性与作用范围。
     */
    enum class SessionsType
    {
        Private = 1, ///< 私聊会话（仅参与者可见）
        Channel = 2, ///< 频道会话（频道内可见）
        Server = 3   ///< 服务器会话（整个服务器可见）
    };

    /**
     * @brief 用户管理接口类
     *
     * 提供从用户标识符（ID）与用户名称之间的互相查询能力。
     * 用于在命令解析时快速获取用户信息。
     */
    class IUserManager
    {
    public:
        virtual ~IUserManager() = default;

        /**
         * @brief 通过用户ID获取用户名称
         *
         * @param ServerConnectionHandler 服务器连接句柄
         * @param userID 用户唯一标识符
         * @return 对应的用户名称，若未找到则返回空字符串
         */
        virtual std::string GetUserName(uint64_t ServerConnectionHandler,ID userID) = 0;

        /**
         * @brief 通过用户名（支持部分匹配）搜索用户ID
         *
         * @param ServerConnectionHandler 服务器连接句柄
         * @param userName 待搜索的用户名子串
         * @return 匹配的用户ID到用户名的映射表
         */
        virtual std::unordered_map<ID, std::string> SearchUserID(uint64_t ServerConnectionHandler, std::string_view userName) = 0;
        
        /**
         * @brief 获取所有已知用户及其对应ID
         *
         * @return 所有用户的ID到名称映射表
         */
        virtual std::unordered_map<ID, std::string> GetAllUsers() = 0;
    };

}

namespace Sessions::SessionTemp
{
    /**
     * @brief 会话抽象接口（临时命名空间）
     *
     * `Session` 代表一个独立的交互上下文，例如一个跑团房间或一个多人游戏会话。
     * 该接口定义了会话的基本属性访问器与修改器，以及用户/管理员管理方法。
     *
     * 每个会话拥有独立的骰子系统实例（通过 `GetDiceSystem()` 获取）。
     *
     * @note 该接口禁止拷贝，确保会话资源的唯一所有权。
     *       实际对象由会话管理器创建。
     *
     * @see SessionManagerTemp::SessionManager
     */
    class Session
    {
    public:
        Session() = default;
        
        ~Session() = default;

        //禁止拷贝和赋值
        Session(const Session&) = delete;
        Session operator=(const Session&) = delete;

        virtual const ID GetID() const = 0;
        virtual const uint64 GetCreateID() const = 0;
        virtual const uint64 GetChannelID() const = 0;
        virtual const uint64 GetServerHandler() const = 0;
        virtual const std::string_view GetServerUUID() const = 0;
        virtual const std::string_view GetCreatorName() const = 0;
        virtual const std::string_view GetCreatorUUID() const = 0;
        virtual const std::string_view GetChannelName() const = 0;
        virtual const std::string_view GetServerName() const = 0;
        virtual const SessionsType GetSessionType() const = 0;
        virtual const std::string_view GetTitle() const = 0;
        virtual const std::string_view GetDescription() const = 0;

        [[deprecated("使用GetUserImpl，此后SessionImpl中只存在带有权限等级的User。")]]
        virtual const std::vector<Command_Core::UserEntity> GetUsers() const = 0;

        virtual const std::vector<Command_Core::UserEntity> GetAdminEntities() const = 0;
        virtual const std::vector<Command_Core::UserEntity> GetUserEntity() const = 0;
        virtual const std::chrono::utc_clock::time_point GetCreateTime() const = 0;
        virtual const std::chrono::utc_clock::time_point GetAliveTime() const = 0;
        virtual const size_t GetUserEntityCount() const = 0;
        virtual const size_t GetAdminCount() const = 0; 


        virtual void SetID(ID sessionID) = 0;
        virtual void SetCreatorID(ID creatorID) = 0;
        virtual void SetChannelID(ID channelID) = 0;
        virtual void SetServerHandler(ID serverHandler) = 0;
        virtual void SetServerUUID(std::string_view serverUUID) = 0;
        virtual void SetCreatorName(std::string_view creatorName) = 0;
        virtual void SetCreatorUUID(std::string_view creatorUUID) = 0;
        virtual void SetChannelName(std::string_view channelName) = 0;
        virtual void SetServerName(std::string_view serverName) = 0;
        virtual void SetSessionType(SessionsType sessionType) = 0;
        virtual void SetTitle(std::string_view title) = 0;
        virtual void SetDescription(std::string_view description) = 0;
        virtual void AddUsers(Command_Core::UserEntity users) = 0;
        virtual void AddUsers(std::vector<Command_Core::UserEntity> users) = 0;
        virtual void AddAdmins(Command_Core::UserEntity admins) = 0;
        virtual void AddAdmins(std::vector<Command_Core::UserEntity> admins) = 0;
        virtual void RemoveUsers(std::vector<Command_Core::UserEntity> users) = 0;
        virtual void RemoveAdmins(std::vector<Command_Core::UserEntity> admins) = 0;


        /**
         * @brief 检查用户是否已存在于当前会话中
         *
         * @param user 待检查的用户对象
         * @return 若用户不存在于会话中则返回 true，否则返回 false
         */
        virtual bool DistinctUsers(Command_Core::UserEntity& user) = 0;

        /**
         * @brief 获取此会话绑定的骰子系统实例
         *
         * @return 指向 `IDiceSystem` 的独占指针的引用
         */
        virtual std::unique_ptr<IDiceSystem>& GetDiceSystem() = 0;
        
    };


}


namespace Sessions::SessionManagerTemp
{
    using SessionsPtr = std::unique_ptr<SessionTemp::Session>;

    //SessionManager
    //TODO:修改SessionManager为单例模式

    /**
     * @brief 会话管理器抽象接口（临时命名空间）
     *
     * `SessionManager` 负责管理所有活动会话的生命周期，并提供基于不同策略的会话获取方法。
     * 它是连接用户命令与具体会话对象的桥梁，同时也是 `IDiceContextProvider` 的天然实现者。
     *
     * 主要功能：
     * - **会话生命周期管理**：`CreateSession`、`EndSession`、`InitSession`
     * - **用户/管理员管理**：`AddUser`、`RemoveUser`、`SetAdmin` 等
     * - **会话获取**：通过 `GetSession` / `GetSessionsList` 按多种方式查找
     * - **用户选择状态管理**：维护一个从用户 UUID 到当前选中会话 ID 的映射
     *
     * @note 该接口位于 `SessionManagerTemp` 命名空间，表明其设计可能随会话模块重构而变化。
     *       允许移动构造/赋值，以支持工厂返回或容器存储。
     *
     * @see Session, SessionFetchMethod, SaiKoLily::IDiceContextProvider
     */
    class SessionManager : public SaiKoLily::IDiceContextProvider
    {
    public:
        virtual ~SessionManager() = default;

        SessionManager() = default;

        //禁止拷贝
        SessionManager(const SessionManager&) = delete;
        SessionManager& operator=(const SessionManager&) = delete;

    public:

        // ----- 生命周期管理 -----
        /**
         * @brief 创建一个新的会话
         *
         * @param context 命令上下文，用于获取创建者信息及日志
         * @return 新创建会话的唯一标识符（ID）
         */
        virtual ID CreateSession(const Command_Core::ICommandContext& context) = 0;

        /**
         * @brief 结束并销毁指定会话
         *
         * @param sessionID 待销毁的会话 ID
         */
        virtual void EndSession(ID sessionID) = 0;

        /**
         * @brief 初始化已创建但尚未配置的会话
         *
         * 通常在 `CreateSession` 之后调用，用于设置标题、描述等元数据。
         *
         * @param sessionID 会话 ID
         * @param context 命令上下文
         * @param title 会话标题
         * @param description 会话描述
         * @return 初始化成功返回 true，否则返回 false
         */
        virtual bool InitSession(ID sessionID, const Command_Core::ICommandContext& context, std::string_view title, std::string_view description) = 0;
        
        // ----- 持久化 -----
        /**
         * @brief 保存指定会话的配置信息
         *
         * @param sessionID 会话 ID
         */
        virtual void SaveSession(ID sessionID) = 0;

        /**
         * @brief 加载指定会话的配置信息
         *
         * @param sessionID 会话 ID
         * @return 加载成功返回 true，否则返回 false
         */
        virtual bool LoadSession(ID sessionID) = 0;

        // ----- 用户与管理员管理 -----

        /**
         * @brief 添加用户到指定会话
         *
         * @param sessionID 会话 ID
         * @param user 待添加的用户对象
         * @param context 命令上下文
         * @return 添加成功返回 true，否则返回 false
         */
        virtual bool AddUser(ID sessionID, std::vector<Command_Core::UserEntity> user, const Command_Core::ICommandContext& context) = 0;

        /**
         * @brief 移除用户从指定会话
         *
         * @param sessionID 会话 ID
         * @param user 待移除的用户对象
         * @param context 命令上下文
         * @return 移除成功返回 true，否则返回 false
         */
        virtual bool RemoveUser(ID sessionID, std::vector<Command_Core::UserEntity> user, const Command_Core::ICommandContext& context) = 0;

        /**
         * @brief 设置用户为管理员
         *
         * @param sessionID 会话 ID
         * @param user 待设置管理员的用户对象
         * @param context 命令上下文
         * @return 设置成功返回 true，否则返回 false
         */
        virtual bool SetAdmin(ID sessionID, std::vector<Command_Core::UserEntity> user, const Command_Core::ICommandContext& context) = 0;

        /**
         * @brief 移除会话中的管理员
         *
         * @param sessionID 会话 ID
         * @param user 待移除的管理员的用户对象
         * @param context 命令上下文
         * @return 移除成功返回 true，否则返回 false
         */
        virtual bool RemoveAdmin(ID sessionID, std::vector<Command_Core::UserEntity> user, const Command_Core::ICommandContext& context) = 0;

        /**
         * @brief 获取指定会话中的所有用户
         *
         * @param sessionID 会话 ID
         * @param context 命令上下文
         * @return 包含所有用户的向量
         */
        virtual std::vector<Command_Core::UserEntity> GetUsers(ID sessionID, const Command_Core::ICommandContext& context)  = 0;

        /**
         * @brief 获取指定会话中的所有管理员
         *
         * @param sessionID 会话 ID
         * @param context 命令上下文
         * @return 包含所有管理员的用户向量
         */
        virtual std::vector<Command_Core::UserEntity> GetAdmins(ID sessionID, const Command_Core::ICommandContext& context) = 0;

        /**
         * @brief 获取当前所有已经存在的会话ID。
         *
         * @param context 命令上下文
         * @return 包含所有会话ID的向量
         */
        virtual std::vector<ID> GetSessionLists () = 0;

        /**
         * @brief 暂停指定会话的活动状态
         * 
         * @param sessionID 需要被暂停的会话ID
         */
        virtual void PauseSession(ID sessionID) = 0;

        /**
         * @brief 恢复指定会话的活动状态
         * 
         * @param sessionID 需要被恢复的会话ID
         */
        virtual void ResumeSession(ID sessionID) = 0;


        // ----- 用户选择状态 -----

        /**
         * @brief 获取用户选择的会话ID
         *
         * 获取指定用户当前选择的会话ID，用于支持用户->Session的查询。
         * 
         * @param user 执行选择的用户
         * @return 用户选择的会话ID，若无选择则返回 0
         */
        virtual ID GetSelectedSessionOfUser(const Command_Core::ICommandContext& user) = 0;

        /**
         * @brief 获取用户 UUID 到当前选中会话 ID 的映射表（只读）
         *
         * 该映射记录了每个用户当前“激活”的会话，用于支持 `SessionFetchMethod::Selected` 查询。
         *
         * @return 映射的常量引用
         */
        virtual const std::unordered_map<std::string, std::unordered_map<std::string, ID>>& GetSelectionOfSession() const = 0;

        /**
         * @brief 用户选择（激活）一个会话
         *
         * 将用户与指定会话关联，后续命令若未明确指定会话，将默认操作此会话。
         *
         * @param session 被选择的会话对象
         * @param user 执行选择的用户
         * 
         * @warning 选择会话前应确保用户已加入该会话，否则可能导致不一致状态。
         * @warning 选择会话优先级：1. 用户主动选择会话 > 2.用户加入会话自动选择最后加入的会话 > 3.频道绑定的会话
         */
        virtual void UserSelectSession(const SessionTemp::Session& session, const Command_Core::UserEntity& user) = 0;

        /**
         * @brief 用户取消选择（取消激活）一个会话
         *
         * 取消用户与指定会话的关联，后续命令若未明确指定会话，将默认操作其他会话。
         *
         * @param session 被取消选择的会话对象
         * @param user 执行取消选择的用户
         */
        virtual void UserUnselectSession(const SessionTemp::Session& session, const Command_Core::UserEntity& user) = 0;

        // ----- 实现 IDiceContextProvider -----
        
        /**
         * @brief 根据命令上下文获取对应的骰子系统实例
         *
         * 根据用户当前选中的会话、用户所在的会话、频道绑定的会话等优先级规则，
         * 查找并返回当前上下文关联的骰子系统。
         *
         * @param context 命令执行上下文
         * @return 指向关联 `IDiceSystem` 的指针，若无关联则返回 `nullptr`
         */
        SaiKoLily::DiceSystem::IDiceSystem* GetDiceSystem(const Command_Core::ICommandContext& context) override = 0;

    public:

        // ----- 会话查询（多种过滤方式）-----

        /**
         * @brief 根据过滤方式获取会话列表
         *
         * @param method 过滤方式（按创建者(Creator)、服务器(Server)、频道(Channel)）
         * @param arg 用于参与过滤的参数（如创建者名称、服务器名称等）
         * @return 匹配的会话引用包装器向量
         */
        virtual std::vector<std::reference_wrapper<SessionTemp::Session>> GetSessionsList(Sessions::SessionListFilter& method, std::string_view arg) = 0;
        
        /**
         * @brief 获取所有活动会话列表（无过滤）
         *
         * @return 所有会话的引用包装器向量
         */
        virtual std::vector<std::reference_wrapper<SessionTemp::Session>> GetSessionsList() = 0;

        /**
         * @brief 根据获取方式和参数查找单个会话
         *
         * @param method 获取方式（按标题(Title)、创建者(Creator)、ID）
         * @param arg 查找参数
         * @return 若找到则返回会话指针包装在 `std::optional` 中，否则返回 `std::nullopt`
         */
        virtual std::optional<SessionTemp::Session*> GetSession(Sessions::SessionFetchMethod& method, std::string_view arg) = 0;

        /**
         * @brief 根据获取方式和参数查找会话列表
         * 
         * @param method 获取方式（按标题(Title)、创建者(Creator)、ID）
         * @param arg 查找参数
         * @return std::optional<std::vector<std::reference_wrapper<SessionTemp::Session>>> 若找到则返回会话引用包装器向量，否则返回 std::nullopt
         */
        virtual std::optional<std::vector<std::reference_wrapper<SessionTemp::Session>>> GetSessionsList(Sessions::SessionFetchMethod& method, std::string_view arg) = 0;
    };


}


namespace Sessions::SessionsCommandTemp
{
    /*
        创建会话，结束会话，查询会话信息，查询会话列表，搜索会话，
        激活会话，会话休眠，用户选择会话，用户解除选择会话，用户加入，
        用户退出，设置管理员，移除管理员，管理员列表，用户列表，
        绑定会话至，消息记录日志，删除消息记录日志，修改会话，导入会话，
        导出会话，命令传入。
    */
    
    /**
     * @brief 会话命令解析项类型枚举
     *
     * 对应每个具体的子命令，用于解析和执行。
     */
    enum class TermType
    {
        Create = 1, End, Status, Info, List, Search, Activate, Rest, Select, UnSelect, UserJoin, UserQuit, AdminsSet, AdminsRemove, AdminsList, UserList, Bind, History, CleanHistory, SaveToFile, Set, Export, Import, Help, Others, NULLCommand
    };

    /**
     * @brief 会话获取方式字符串到枚举值的映射表
     * 
     * 等待后续使用更强大的匹配方法。
     */
    static std::unordered_map<std::string_view, Sessions::SessionFetchMethod> FetchMethodMap =
    {
        {"Index", Sessions::SessionFetchMethod::Index},
        {"index", Sessions::SessionFetchMethod::Index},
        {"ID", Sessions::SessionFetchMethod::Index},
        {"id", Sessions::SessionFetchMethod::Index},
        {"i", Sessions::SessionFetchMethod::Index},
        {"Title", Sessions::SessionFetchMethod::Title},
        {"title", Sessions::SessionFetchMethod::Title},
        {"t", Sessions::SessionFetchMethod::Title},
        {"Creator", Sessions::SessionFetchMethod::Creator},
        {"creator", Sessions::SessionFetchMethod::Creator},
        {"C", Sessions::SessionFetchMethod::Creator}
    };

    /**
     * @brief 命令解析项结构体
     *
     * 表示解析后的一个子命令及其参数列表。
     * @warning 唯一性的命令。
     */
    struct ExpressionTerm
    {
        using Args = std::string;
        TermType type = TermType::NULLCommand; ///< 子命令类型
        std::vector<Args> args;                ///< 子命令参数列表
    };



    /**
     * @brief 会话命令处理类
     *
     * 实现 `ICommandHandler`，负责解析以 "sessions" 或 "ss" 等别名开头的命令，
     * 并调用 `SessionManager` 执行相应的会话管理操作。
     *
     * @see Command_Core::ICommandHandler
     */
    class SessionsCommand : public Command_Core::ICommandHandler
    {
    public:
        SessionsCommand() = default;
        
        ~SessionsCommand() = default;

        //禁止移动与拷贝构造
        SessionsCommand(SessionsCommand& other) = delete;
        SessionsCommand& operator=(SessionsCommand& other) = delete;

    public:
        //由其派生类来实现
        //virtual void Execute(Command_Core::ICommandContext& context) = 0;

        std::string_view GetName() const override
        {
            return "Sessions";
        }
        std::string_view GetDescription() const override
        {
            return "会话管理命令\"Session\"。用来管理频道会话，可以用来创建会话并在会话中记录各种信息与提取会话包含的信息。";
        }
        std::vector<std::string_view> GetAlias() const override
        {
            return {"session", "sessions", "game", "Game", "Session", "Sessions", "ss", "SS"};
        }

    public:

        /**
         * @brief 获取关联的会话管理器实例
         *
         * @return 指向 `SessionManager` 的原始指针
         */
        virtual Sessions::SessionManagerTemp::SessionManager* GetSessionsManager() = 0;

    protected:

        /**
         * @brief 解析命令参数字符串为命令项列表
         *
         * 将输入的字符串向量（如 ["create", "MyGame", "desc"]）解析为
         * `ExpressionTerm` 列表（如 `{ TermType::Create, {"MyGame", "desc"} }`）。
         *
         * @param exprs 命令参数字符串视图向量
         * @param context 命令上下文（用于调试日志）
         * @return 解析后的命令项列表
         */
        static std::vector<ExpressionTerm> ParserExpression(std::vector<std::string_view> exprs,
                                                             Command_Core::ICommandContext& context)
        {
            std::vector<ExpressionTerm> Terms;

            #ifdef _DEBUG
            context.Log("命令解析前未出错", Plugin_Logs::logLevel::info, true);

            for(auto& expr : exprs)
            {
                context.Log(std::format("Sessions: 参数：{}", expr), Plugin_Logs::logLevel::info, false);
            }
            #endif

            if(exprs.empty())
            {
                #ifdef _DEBUG
                context.Log("Sessions: 命令为空！", Plugin_Logs::logLevel::warn, false);
                #endif
                return Terms;
            }

            for(auto& expr : exprs)
            {
                if(commands.contains(expr))
                {
                    Terms.emplace_back(commands[expr], std::vector<ExpressionTerm::Args>{});
                }
                else if(!commands.contains(expr) && !Terms.empty())
                {
                    #ifdef _DEBUG
                    context.Log(std::format("添加参数：{}", expr), Plugin_Logs::logLevel::info, false);
                    #endif
                    Terms.back().args.push_back(std::string(expr));
                }
                else
                {
                    // 未识别命令：只提示一次并终止解析，避免逐 token 刷屏。
                    context.Log(std::format("Sessions: 命令 {} 不存在！", expr), Plugin_Logs::logLevel::warn, false);
                    context.SendResult(std::format("Sessions: 未识别的命令：{}", expr),
                                       Command_Core::MessageTarget::CurrentChannel);
                    return {};
                }
            }

            if(Terms.empty())
            {
                context.Log("Sessions: 命令为空！", Plugin_Logs::logLevel::warn, false);
                context.SendResult("Sessions: 缺少会话命令。",
                                   Command_Core::MessageTarget::CurrentChannel);
                return Terms;
            }

            #ifdef _DEBUG
            for(auto& term : Terms)
            {
                if(term.type == TermType::NULLCommand)
                {
                    context.Log(std::format("{} 命令为空！", static_cast<int>(term.type)),
                                Plugin_Logs::logLevel::warn, false);
                }
                else
                {
                    context.Log(std::format("Sessions: 命令 {} 已被解析！", static_cast<int>(term.type)),
                                Plugin_Logs::logLevel::info, false);
                }
            }

            context.Log("命令解析完毕", Plugin_Logs::logLevel::info, true);
            #endif

            return Terms;
        }

        /**
         * @brief 由命令上下文构造当前调用者的 User（UUID 为必填，clientID 仅作传输快照）
         */
        static Command_Core::UserEntity BuildCallingUser(const Command_Core::ICommandContext& context)
        {
            auto info = context.GetUserByName(context.GetCallingUserName());

            return Command_Core::UserEntity(
                info.nickName.empty() ? std::string(context.GetCallingUserName()) : info.nickName,
                info.channelID == 0 ? context.GetChannelID() : info.channelID,
                context.GetServerHandler(),
                info.clientID == 0 ? context.GetCallingClientID() : info.clientID,
                info.uniqueID.empty() ? std::string(context.GetCallingUserUUID()) : info.uniqueID,
                Command_Core::UserLevel::NonMember
            );
        }

        //鉴权
        static Command_Core::UserLevel GetUserLevel(TermType command, const Command_Core::ICommandContext& context, const Sessions::SessionTemp::Session* session)
        {
            if(!session)
                return Command_Core::UserLevel::NonMember;

            const std::string_view userUUID = context.GetCallingUserUUID();
            const ID userID = context.GetCallingClientID();
            const std::string_view creatorUUID = session->GetCreatorUUID();

            // Creator 以 UUID 为准；仅有旧会话（creatorUUID 为空）时退回 clientID 比对。
            if((!userUUID.empty() && !creatorUUID.empty() && creatorUUID == userUUID) ||
               (creatorUUID.empty() && session->GetCreateID() == userID))
                return Command_Core::UserLevel::Creator;
            
            const auto adminsList = session->GetAdminEntities();

            if(!adminsList.empty())
            {
                for(const auto& member : adminsList)
                {
                    if(!userUUID.empty() && member.uuid == userUUID)
                        return Command_Core::UserLevel::Admins;
                }
            }

            const auto userList = session->GetUsers();
            if(!userList.empty())
            {
                for(const auto& member : userList)
                {
                    if(!userUUID.empty() && member.uuid == userUUID)
                        return Command_Core::UserLevel::Member;
                }
            }


            
            return Command_Core::UserLevel::NonMember;
        }

        /**
         * @brief 执行解析后的命令项列表
         *
         * 遍历命令项，调用 `SessionManager` 的相应接口执行实际操作。
         *
         * @param terms 待执行的命令项列表
         * @param manager 会话管理器实例
         * @param context 命令上下文
         * @return 执行结果描述字符串
         */
        static std::string ExecuteExpression(const std::vector<Sessions::SessionsCommandTemp::ExpressionTerm>& terms, Sessions::SessionManagerTemp::SessionManager* manager, Command_Core::ICommandContext& context)
        {
            //首先先查询并获取该用户在SessionManager中的SelectedSessionID映射。
            ID UserSelectedSession = manager->GetSelectedSessionOfUser(context);
            SessionTemp::Session* WorkingSession = nullptr;

            //替换使用新逻辑，更加简洁易懂。
            if(UserSelectedSession != 0)
            {
                auto method = Sessions::SessionFetchMethod::Index;

                WorkingSession = manager->GetSession(method, std::to_string(UserSelectedSession)).value_or(nullptr);
            }

            //刷新工作会话：Create / Select / UnSelect / End 之后统一调用。
            auto RefreshWorkingSession = [&]()
            {
                UserSelectedSession = manager->GetSelectedSessionOfUser(context);

                if(UserSelectedSession == 0)
                {
                    WorkingSession = nullptr;
                    return;
                }

                auto method = Sessions::SessionFetchMethod::Index;
                WorkingSession = manager->GetSession(method, std::to_string(UserSelectedSession)).value_or(nullptr);
            };

            //服务器作用域校验：目标会话必须属于当前 serverUUID（映射未建立时放行）。
            auto IsSameServer = [&](const SessionTemp::Session* session)
            {
                const auto serverUUID = context.GetServerUUID();
                return session != nullptr && (serverUUID.empty() || session->GetServerUUID() == serverUUID);
            };

            //将所有Select移动至开头防止多次重复定义。
            auto Copy_Of_Terms = terms;
            std::stable_partition(Copy_Of_Terms.begin(), Copy_Of_Terms.end(), [](const auto& term) { 
                return term.type == TermType::Select;
            });
            //防止多次选择不同Session，最后一个Select会被最终执行。

            for(auto& term : Copy_Of_Terms)
            {
                context.Log(std::format("Sessions: 命令 {} 开始执行", static_cast<int>(term.type)), Plugin_Logs::logLevel::info, true);

                //命令识别
                const auto requiredLevelIt = TermUserLevels.find(term.type);
                if(requiredLevelIt == TermUserLevels.end())
                    return "Sessions: 未知或未实现的命令。";

                //用户鉴权
                const Command_Core::UserLevel callerLevel = GetUserLevel(term.type, context, WorkingSession);
                if(static_cast<uint8_t>(callerLevel) > static_cast<uint8_t>(requiredLevelIt->second))
                    return std::format("Sessions: 权限不足（当前等级 {}，需要等级 {}）。",
                                       static_cast<int>(callerLevel),
                                       static_cast<int>(requiredLevelIt->second));

                switch(term.type)
                {
                    case TermType::Create: 
                        {   
                            //统一解析标题与描述，避免多分支重复创建。
                            std::string title = "<Unknown Title>";
                            std::string description = "No Description.";

                            if(term.args.size() >= 1)
                                title = term.args[0];
                            if(term.args.size() >= 2)
                                description = term.args[1];

                            auto sessionID = manager->CreateSession(context);
                            manager->InitSession(sessionID, context, title, description);

                            //将创建者加入会话，避免“创建后无工作会话”的死锁。
                            Command_Core::UserEntity creator = BuildCallingUser(context);

                            manager->AddUser(sessionID, { creator }, context);

                            auto method = Sessions::SessionFetchMethod::Index;
                            auto newSession = manager->GetSession(method, std::to_string(sessionID));

                            if(!newSession.has_value())
                            {
                                manager->EndSession(sessionID);
                                return "Sessions: 创建会话失败！";
                            }

                            newSession.value()->AddAdmins(creator);
                            manager->UserSelectSession(*newSession.value(), creator);

                            //刷新当前命令上下文中的工作会话。
                            RefreshWorkingSession();
                        }
                        break;
                    case TermType::End: 
                        {
                            if(term.args.size() == 0)
                            {
                                //默认删除用户选择的会话。
                                if(UserSelectedSession == 0)
                                {
                                    return "SessionsEnd: 没有选择会话！";
                                }

                                manager->EndSession(UserSelectedSession);
                                RefreshWorkingSession();
                                break;
                            }
                            
                            if(term.args.size() != 2)
                            {
                                return "SessionsEnd: 参数错误！所需的参数数量必须为 0 或 2。";
                            }
                            auto method = FetchMethodMap.at(term.args[0]);

                            auto session_ = manager->GetSession(method, term.args[1]);

                            if(session_.value_or(nullptr) == nullptr)
                            {
                                return "SessionsEnd: 获取会话失败！";
                            }

                            if(!IsSameServer(session_.value()))
                            {
                                return "SessionsEnd: 该会话不属于当前服务器！";
                            }

                            const ID endedSessionID = session_.value()->GetID();
                            manager->EndSession(endedSessionID);

                            if(UserSelectedSession == endedSessionID)
                                RefreshWorkingSession();
                        }
                        break;
                    case TermType::Status:
                        {
                            context.Log(std::format("Sessions:获取当前状态"), Plugin_Logs::logLevel::info, true);

                            std::string result;



                            context.SendResult(result, context.GetMessageTarget());
                        }
                        break;
                    case TermType::Info:
                        {
                            context.Log(std::format("Sessions:获取会话信息"), Plugin_Logs::logLevel::info, true);

                            Sessions::SessionTemp::Session * info;

                            if(UserSelectedSession == 0)
                            {
                                auto method = Sessions::SessionFetchMethod::Creator;
                                info = manager->GetSession(method, context.GetCallingUserName()).value_or(nullptr);
                                if(info == nullptr)
                                {
                                    return "Sessions: 没有会话！";
                                }
                            }
                            else
                            {
                                info = WorkingSession;
                            }
                            

                            if(info != nullptr)
                            {
                                std::string session_type;
                                switch (info->GetSessionType())
                                {
                                case Sessions::SessionsType::Private:
                                    session_type = "Private";
                                    break;
                                case Sessions::SessionsType::Channel:
                                    session_type = "Channel";
                                    break;
                                case Sessions::SessionsType::Server:
                                    session_type = "Server";
                                    break;
                                default:
                                    break;
                                }

                                // 处理时间点到字符串的转换
                                auto timePoint = info->GetCreateTime();
                                std::time_t timeT = std::chrono::system_clock::to_time_t(
                                    std::chrono::system_clock::from_time_t(0) + 
                                    std::chrono::duration_cast<std::chrono::system_clock::duration>(
                                        timePoint.time_since_epoch()
                                    )
                                );

                                std::string createTimeStr = std::ctime(&timeT);
                                // 移除ctime添加的换行符
                                if(!createTimeStr.empty() && createTimeStr.back() == '\n') {
                                    createTimeStr.pop_back();
                                }

                                context.SendResult(std::format("Sessions: 获取会话信息成功！\n会话ID: {}\n会话标题: {}\n会话描述: {}\n会话类型: {}\n会话创建时间: {}\n会话创建者: {}\n会话创建者ID: {}", info->GetID(), info->GetTitle(), info->GetDescription(), session_type, createTimeStr, info->GetCreatorName(), info->GetCreateID()), context.GetMessageTarget());

                                std::string user_info_string = [&]()
                                    {
                                        std::string result;
                                        for(auto&& user : info->GetUsers())
                                        {
                                            result += std::format("{}({})\t\t", user.name, user.userID);
                                        }
                                        return result;
                                    }();

                                context.SendResult(std::format("会话中人数：{}, 会话用户：\n{}\n", info->GetUserEntityCount(), user_info_string), context.GetMessageTarget());
                            }
                            else
                            {
                                return "Sessions: 获取会话信息失败！";
                            }
                        }
                        break;
                    case TermType::List: 
                        {
                            auto list = manager->GetSessionLists();

                            std::string result;

                            auto method_id = Sessions::SessionFetchMethod::Index;

                            for (auto &&iter : list)
                            {
                                result += std::format("SessionsID: {} \t SessionsName: {}\n", iter, manager->GetSession(method_id, std::to_string(iter)).has_value() ? manager->GetSession(method_id, std::to_string(iter)).value()->GetTitle() : "SessionList : 未知错误。");
                            }
                            
                            context.Log(std::format("Sessions: 获取会话列表成功！\n列表：{}", result), Plugin_Logs::logLevel::info, false);
                            context.SendResult(std::format("Sessions: 获取会话列表成功！\n目前的活动列表：\n{}", result), context.GetMessageTarget());
                        }
                        break;
                    case TermType::Search: 
                        {
                            std::string result;

                            //必须获取两个参数，第一个参数是搜索方式，第二个参数是搜索内容。
                            if(term.args.size() != 2)
                            {
                                return "SessionsSearch: 参数错误！所需的参数数量必须为 2。[arg1: 搜索方式, arg2: 搜索内容]。";
                            }

                            //直接调用API获取会话列表并返回结果。
                            auto SessionsList_ = manager->GetSessionsList(FetchMethodMap.at(term.args[0]), term.args[1]);

                            if(SessionsList_.has_value())
                            {
                                for (auto &&iter : SessionsList_.value())
                                {
                                    result += std::format("会话ID: {} \t 会话名称: {}\n 会话简介: {}\n 会话创建者: {}\n 创建时间: {}\n", iter.get().GetID(), iter.get().GetTitle(), iter.get().GetDescription(), iter.get().GetCreatorName(), iter.get().GetCreateTime());
                                }
                                context.Log(std::format("Sessions: 搜索会话成功！\n列表：{}", result), Plugin_Logs::logLevel::info, false);
                                context.SendResult(std::format("Sessions: 搜索会话成功！\n搜索结果：\n{}", result), context.GetMessageTarget());
                            }
                            else
                            {
                                return "SessionsSearch: 搜索会话失败！";
                            }
                            
                        }
                        break;
                    case TermType::Activate: 
                        break;
                    case TermType::Rest: 
                        break;
                    case TermType::Select:
                        {
                            if(term.args.size() != 2)
                            {
                                return "SessionsSelect: 参数不足！需要 [方式, 参数]。";
                            }

                            //通过参数获取查找方式
                            auto method = FetchMethodMap.at(term.args[0]);

                            //获取会话
                            auto session_ = manager->GetSession(method, term.args[1]);

                            if(!session_.has_value())
                            {
                                return "SessionsSelect: 获取会话失败！";
                            }

                            if(!IsSameServer(session_.value()))
                            {
                                return "SessionsSelect: 该会话不属于当前服务器！";
                            }

                            //获取并构造一个用户
                            Command_Core::UserEntity user = BuildCallingUser(context);

                            //添加用户UUID到被选择Session的映射（内部会校验成员资格）
                            manager->UserSelectSession(*session_.value(), user);

                            //选择成功后刷新工作会话
                            RefreshWorkingSession();
                        }
                        break;
                    case TermType::UnSelect:
                        {
                            if(term.args.size() > 2)
                                return "SessionsSelect: 参数过多！";

                            Command_Core::UserEntity user = BuildCallingUser(context);

                            if(term.args.empty())
                            {
                                //取消当前选择
                                const ID selectedSessionID = manager->GetSelectedSessionOfUser(context);
                                if(selectedSessionID == 0)
                                    return "SessionsSelect: 当前没有选择任何会话！";

                                auto method = Sessions::SessionFetchMethod::Index;
                                auto selectedSession = manager->GetSession(method, std::to_string(selectedSessionID));
                                if(!selectedSession.has_value())
                                    return "SessionsSelect: 会话不存在！";

                                manager->UserUnselectSession(*selectedSession.value(), user);
                                RefreshWorkingSession();
                                break;
                            }

                            if(term.args.size() != 2)
                                return "SessionsSelect: 参数不足！需要 [方式, 参数]。";

                            auto method = FetchMethodMap.at(term.args[0]);

                            //获取会话
                            auto session_ = manager->GetSession(method, term.args[1]);

                            if(!session_.has_value())
                            {
                                return "SessionsSelect: 获取会话失败！";
                            }

                            if(!IsSameServer(session_.value()))
                            {
                                return "SessionsSelect: 该会话不属于当前服务器！";
                            }

                            manager->UserUnselectSession(*session_.value(), user);
                            RefreshWorkingSession();
                        }
                        break;
                    case TermType::UserJoin:
                        {
                            std::vector<Command_Core::UserEntity> users;
                            std::string result = "";

                            Sessions::SessionTemp::Session* workingSession = WorkingSession;
                            if(workingSession == nullptr)
                            {
                                return "Sessions: 当前没有工作会话！";
                            }

                            //每一个arg是一个字符串类型的用户名
                            for(auto& user : term.args)
                            {
                                #ifdef _DEBUG
                                    context.Log(std::format("查找用户：{}", user), Plugin_Logs::logLevel::info, false);
                                #endif
                                auto tmp = context.GetUserByName(user);
                                if(tmp.uniqueID == "")
                                {
                                    result += std::format("Sessions: 未查询到用户：{}！\n", user);
                                    continue;
                                }
                                Command_Core::UserEntity newUser(tmp.nickName, tmp.channelID, context.GetServerHandler(), tmp.clientID, tmp.uniqueID);

                                //创建逻辑用户对象
                                users.push_back(newUser);
                                #ifdef _DEBUG
                                    context.Log(std::format("Sessions: 添加用户 {} 成功！", newUser.name), Plugin_Logs::logLevel::info, false);
                                #endif
                            }

                            manager->AddUser(workingSession->GetID(), users, context);

                            if(!result.empty())
                                context.SendResult(result, context.GetMessageTarget());
                        }
                        break;
                    case TermType::UserQuit:
                        {
                            std::vector<Command_Core::UserEntity> users;
                            std::string result = "";

                            Sessions::SessionTemp::Session* workingSession = WorkingSession;
                            if(workingSession == nullptr)
                            {
                                return "Sessions: 当前没有工作会话！";
                            }
    
                            //每一个arg是一个字符串类型的用户名
                            for(auto& user : term.args)
                            {
                                #ifdef _DEBUG
                                    context.Log(std::format("查找用户：{}", user), Plugin_Logs::logLevel::info, false);
                                #endif
                                auto tmp = context.GetUserByName(user);
                                if(tmp.uniqueID == "")
                                {
                                    result += std::format("Sessions: 未查询到用户：{}！\n", user);
                                    continue;
                                }
                                Command_Core::UserEntity newUser(tmp.nickName, tmp.channelID, context.GetServerHandler(), tmp.clientID, tmp.uniqueID);

                                //创建逻辑用户对象
                                users.push_back(newUser);
                                context.SendResult(std::format("Sessions: 添加待删除的用户 {} 成功！", newUser.name), context.GetMessageTarget());
                            }

                            manager->RemoveUser(workingSession->GetID(), users, context);

                            if(!result.empty())
                                context.SendResult(result, context.GetMessageTarget());
                        }
                        break;
                    case TermType::AdminsSet: 
                        {
                            std::string result;
                            std::vector<Command_Core::UserEntity> Admins;

                            auto* workingSession = WorkingSession;

                            //args：用户名
                            for(auto&user : term.args)
                            {
                                #ifdef _DEBUG
                                    context.Log(std::format("查找用户：{}", user), Plugin_Logs::logLevel::info, false);
                                #endif

                                auto tmp = context.GetUserByName(user);

                                const auto sessionUsers = workingSession->GetUserEntity();
                                auto iter = std::ranges::find_if(sessionUsers, [&tmp](const auto& userEntity){
                                    return userEntity.uuid == tmp.uniqueID;
                                });

                                if(iter == sessionUsers.end())
                                {
                                    result += std::format("Sessions: 未查询到用户：{}！\n", user);
                                    continue;
                                }

                                if(iter->PermissionLevel >= Command_Core::UserLevel::NonMember)
                                {
                                    result += std::format("Sessions: \"{}\"非会话成员，禁止添加！\n", iter->name);
                                    continue;
                                }

                                if(iter->uuid == workingSession->GetCreatorUUID())
                                {
                                    result += std::format("Sessions: 会话创建者(\"{}\")，存在管理员身份！\n", iter->name);
                                    continue;
                                }


                                Admins.push_back(*iter);
                            }

                            workingSession->AddAdmins(Admins);

                            if(!result.empty())
                                context.SendResult(result, context.GetMessageTarget());
                        }
                        break;
                    case TermType::AdminsRemove: 
                        {
                            std::string result;
                            std::vector<Command_Core::UserEntity> removal_list;

                            auto* workingSession = WorkingSession;
                            const auto& Admins = workingSession->GetAdminEntities();

                            for(auto& removal : term.args)
                            {
                                #ifdef _DEBUG
                                    context.Log(std::format("查找用户：{}", removal), Plugin_Logs::logLevel::info, false);
                                #endif

                                auto tmp = context.GetUserByName(removal);

                                auto iter = std::ranges::find_if(Admins, [&tmp](const auto& userEntity) {
                                    return userEntity.uuid == tmp.uniqueID;
                                });

                                if(iter == Admins.end())
                                {
                                    result += std::format("Sessions: 未查询到用户：\"{}\"！\n", removal);
                                    continue;
                                }

                                if(iter->PermissionLevel >= Command_Core::UserLevel::NonMember)
                                {
                                    result += std::format("Sessions: \"{}\"非会话成员，禁止移除！\n", iter->name);
                                    continue;
                                }

                                if(iter->uuid == workingSession->GetCreatorUUID())
                                {
                                    result += std::format("Sessions: \"{}\"是会话创建者，无法移除其管理员身份！\n", iter->name);
                                    continue;
                                }

                                removal_list.push_back(*iter);
                            }

                            workingSession->RemoveAdmins(removal_list);

                            if(!result.empty())
                                context.SendResult(result, context.GetMessageTarget());
                        }
                        break;
                    case TermType::AdminsList: 
                        {
                            const auto* workingSession = WorkingSession;
                            const auto& Admins = WorkingSession->GetAdminEntities();

                            std::string result(std::format("此会话(\"Title: {}, Index: {}\")存在如下管理员：\n", workingSession->GetTitle(), workingSession->GetID()));
                            std::string role;

                            for(auto usr : Admins)
                            {
                                if(usr.PermissionLevel == Command_Core::UserLevel::Admins)
                                {
                                    result += std::format("[NAME:{}, UUID:{}, Role:{}]\n", usr.name, usr.uuid, "Administrator");
                                }
                                else if(usr.PermissionLevel == Command_Core::UserLevel::Creator)
                                {
                                    result += std::format("[NAME:{}, UUID:{}, Role:{}]\n", usr.name, usr.uuid, "Creator");
                                }
                            }

                            if(!Admins.empty())
                                context.SendResult(result, context.GetMessageTarget());
                        }
                        break;
                    case TermType::History:
                        {
                            //Arg0: 无：列出最近 5 条，Arg1：列出最近 N 条（上限 50）。
                            const auto& List = manager->GetDiceSystem(context)->GetDiceEventListConst();

                            size_t limit = 5;
                            if(term.args.size() == 1)
                            {
                                try
                                {
                                    const int requested = std::stoi(term.args[0]);
                                    if(requested <= 0)
                                        return "SessionsHistory: 条数必须大于 0。";

                                    //上限 50，避免一条命令刷出整屏历史。
                                    limit = static_cast<size_t>(std::min(requested, 50));
                                }
                                catch(const std::exception&)
                                {
                                    return "SessionsHistory: 条数必须是数字。用法：.ss history [条数]";
                                }
                            }
                            else if(term.args.size() > 1)
                            {
                                return "SessionsHistory: 参数过多。用法：.ss history [条数]";
                            }

                            const size_t total = List.size();
                            const size_t skip = total > limit ? total - limit : 0;
                            size_t index = 0;

                            std::string result;
                            for(auto&& event : List)
                            {
                                if(index++ < skip)
                                    continue;

                                const std::string time = std::format("{:%Y年%m月%d日%H时%M分%S秒}", event->GetTimeInfo());

                                result += std::format("{} 在 {} 投掷出 ", ResolvePlayerName(*event), time);
                                result += std::format("{}\n", event->ParseExpressionStr());
                                result += std::format("备注: {}\n", event->GetThrowLog().empty() ? "NULL" : event->GetThrowLog());
                            }

                            const std::string title = WorkingSession ? std::string(WorkingSession->GetTitle()) : "当前会话";
                            return std::format("Session: {} 共 {} 条，显示最近 {} 条：\n{}",
                                               title, total, std::min(limit, total), result);
                        }
                        break;
                    case TermType::SaveToFile: 
                        {
                            try
                            {
                                auto& FileManagerHandler = GwongDongFileSystem::FileManager::GetInstance();
                                fs::path path = fs::current_path() / "logs";
                                switch(term.args.size())
                                {
                                case 0: 
                                    {
                                        path = FileManagerHandler.NMakeFile(path, WorkingSession->GetTitle(), "");
                                        break;
                                    }
                                case 1:
                                    {
                                        path = term.args[0];
                                        path = FileManagerHandler.NMakeFile(path, WorkingSession->GetTitle(), "");
                                        break;
                                    }
                                case 2:
                                    {
                                        path = term.args[0];
                                        path = FileManagerHandler.NMakeFile(path, term.args[1], "");
                                        break;
                                    }
                                case 3:
                                    {
                                        path = term.args[0];
                                        path = FileManagerHandler.NMakeFile(path, term.args[1], term.args[2]);
                                        break;
                                    }
                                default: 
                                    return "SessionsSaveToFile: 参数过多或过少！";
                                }

                                const auto& List = manager->GetDiceSystem(context)->GetDiceEventListConst();
                                std::string resultStr;

                                std::string time;
                                //获取全部历史记录
                                for(auto&& event : List)
                                {
                                    //时间获取
                                    time.clear();
                                    time = std::format("{:%Y年%m月%d日%H时%M分%S秒}", event->GetTimeInfo());
                                    
                                    resultStr += std::format("{} 在 {} 投掷出 ", Sessions::ResolvePlayerName(*event), time);
                                    resultStr += std::format("{}\n", event->ParseExpressionStr());
                                    resultStr += std::format("备注: {}\n", event->GetThrowLog().empty() ? "NULL" : event->GetThrowLog());
                                }
                                
                                if(path.empty())
                                {
                                    return "SessionsSaveToFile: 文件创建失败！";
                                }

                                FileManagerHandler.InputBase(path, resultStr, GwongDongFileSystem::WriteMode::Overwrite);

                                return std::format("SessionsSaveToFile: 导出成功，路径：{}", path.string());
                            }
                            catch(const std::exception& e)
                            {
                                return std::format("SessionsSaveToFile: 错误: {}", e.what());
                            }
                        }
                        break;

                    default: 
                        break;
                }
            }

            ///返回执行的错误到 @see void SessionsCommand::Execute(Command_Core::ICommandContext& context) ，此处如果为空，则表示命令执行成功，且不返回任何结果。
            return "";
        }

    private:

        /**
         * @brief 命令字符串到命令类型的映射表
         *
         * 支持中文和英文别名。
         * 
         * @warning 目前的实现是简单的字符串匹配，后续可能需要更复杂的解析方法以支持更灵活的命令输入。
         */
        static inline std::unordered_map<std::string_view, TermType> commands = 
        {
        
            {"Create", TermType::Create},
            {"create", TermType::Create},
            {"New", TermType::Create},
            {"new", TermType::Create},
            {"创建会话", TermType::Create},
            {"新建会话", TermType::Create},
            {"新建", TermType::Create},

            {"End", TermType::End},
            {"end", TermType::End},
            {"Stop", TermType::End},
            {"stop", TermType::End},
            {"结束会话", TermType::End},
            {"结束", TermType::End},
            {"停止会话", TermType::End},
            {"停止", TermType::End},


            {"Status", TermType::Status},
            {"status", TermType::Status},
            {"St", TermType::Status},
            {"st", TermType::Status},
            {"状态", TermType::Status},
            {"查询状态", TermType::Status},

            {"Info", TermType::Info},
            {"info", TermType::Info},
            {"I", TermType::Info},
            {"i", TermType::Info},
            {"查询信息", TermType::Info},
            {"信息", TermType::Info},
            {"会话信息", TermType::Info},

            {"List", TermType::List},
            {"list", TermType::List},
            {"ls", TermType::List},
            {"会话列表", TermType::List},
            {"查询列表", TermType::List},

            {"Search", TermType::Search},
            {"search", TermType::Search},
            {"查询", TermType::Search},
            {"搜索会话", TermType::Search},

            {"Activate", TermType::Activate},
            {"activate", TermType::Activate},
            {"act", TermType::Activate},
            {"激活会话", TermType::Activate},
            {"激活", TermType::Activate},

            {"Rest", TermType::Rest},
            {"rest", TermType::Rest},
            {"Pause", TermType::Rest},
            {"pause", TermType::Rest},
            {"暂停会话", TermType::Rest},
            {"暂停", TermType::Rest},

            {"Select", TermType::Select},
            {"select", TermType::Select},
            {"选择会话", TermType::Select},
            {"选择", TermType::Select},

            {"UnSelect", TermType::UnSelect},
            {"unselect", TermType::UnSelect},
            {"取消选择", TermType::UnSelect},

            {"UserJoin", TermType::UserJoin},
            {"userjoin", TermType::UserJoin},
            {"Join", TermType::UserJoin},
            {"Add", TermType::UserJoin},
            {"Usr", TermType::UserJoin},
            {"添加用户", TermType::UserJoin},
            {"加入用户", TermType::UserJoin},

            {"UserQuit", TermType::UserQuit},
            {"userquit", TermType::UserQuit},
            {"DeleteUser", TermType::UserQuit},
            {"Delete", TermType::UserQuit},
            {"Quit", TermType::UserQuit},
            {"退出用户", TermType::UserQuit},
            {"退出", TermType::UserQuit},
            {"删除用户", TermType::UserQuit},
            {"删除", TermType::UserQuit},
            {"移除用户", TermType::UserQuit},
            {"移除", TermType::UserQuit},
            {"用户退出", TermType::UserQuit},


            {"AdminsSet", TermType::AdminsSet},
            {"adminset", TermType::AdminsSet},
            {"SetAdmins", TermType::AdminsSet},
            {"setadmins", TermType::AdminsSet},
            {"设置管理员", TermType::AdminsSet},
            {"管理员", TermType::AdminsSet},

            {"AdminsRemove", TermType::AdminsRemove},
            {"adminsremove", TermType::AdminsRemove},
            {"RemoveAdmins", TermType::AdminsRemove},
            {"removeadmins", TermType::AdminsRemove},
            {"移除管理员", TermType::AdminsRemove},
            {"移除管理", TermType::AdminsRemove},

            {"AdminsList", TermType::AdminsList},
            {"adminslist", TermType::AdminsList},
            {"ListAdmins", TermType::AdminsList},
            {"listadmins", TermType::AdminsList},
            {"管理员列表", TermType::AdminsList},
            {"查询管理员", TermType::AdminsList},

            {"UserList", TermType::UserList},
            {"userlist", TermType::UserList},
            {"ListUsers", TermType::UserList},
            {"listusers", TermType::UserList},
            {"用户列表", TermType::UserList},

            {"Bind", TermType::Bind},
            {"bind", TermType::Bind},
            {"绑定", TermType::Bind},

            {"History", TermType::History},
            {"history", TermType::History},
            {"历史", TermType::History},
            {"查看历史", TermType::History},
            {"查询历史", TermType::History},

            {"STF", TermType::SaveToFile},
            {"SaveToFile", TermType::SaveToFile},
            {"savetofile", TermType::SaveToFile},
            {"保存历史", TermType::SaveToFile},
            {"保存", TermType::SaveToFile},
            {"保存会话数据", TermType::SaveToFile},
            {"保存会话记录", TermType::SaveToFile},

            {"CleanHistory", TermType::CleanHistory},
            {"cleanhistory", TermType::CleanHistory},
            {"清除历史", TermType::CleanHistory},
            {"删除历史记录", TermType::CleanHistory},
            {"删除历史", TermType::CleanHistory},

            {"Set", TermType::Set},
            {"set", TermType::Set},
            {"设置", TermType::Set},
            {"修改设置", TermType::Set},

            {"Export", TermType::Export},
            {"export", TermType::Export},
            {"导出会话数据", TermType::Export},
            {"导出会话记录", TermType::Export},

            {"Import", TermType::Import},
            {"import", TermType::Import},
            {"导入会话记录", TermType::Import},
            {"导入会话数据", TermType::Import},

            {"Help", TermType::Help},
            {"help", TermType::Help},
            {"GetHelp", TermType::Help},
            {"gethelp", TermType::Help},
            {"帮助", TermType::Help},
            {"获取帮助", TermType::Help},
            {"使用帮助", TermType::Help},
            {"查询帮助", TermType::Help},
            {"使用方法", TermType::Help},
            {"使用说明", TermType::Help},
            {"帮助文档", TermType::Help},

            {"Others", TermType::Others}
        
        };
        


        static inline std::unordered_map<TermType, Command_Core::UserLevel> TermUserLevels =
        {
            {TermType::Create, Command_Core::UserLevel::NonMember},
            {TermType::End, Command_Core::UserLevel::Admins},
            {TermType::Status, Command_Core::UserLevel::Member},
            {TermType::Info, Command_Core::UserLevel::NonMember},
            {TermType::List, Command_Core::UserLevel::NonMember},
            {TermType::Search, Command_Core::UserLevel::NonMember},
            {TermType::Activate, Command_Core::UserLevel::Admins},
            {TermType::Rest, Command_Core::UserLevel::Admins},
            {TermType::Select, Command_Core::UserLevel::NonMember},
            {TermType::UnSelect, Command_Core::UserLevel::NonMember},
            {TermType::UserJoin, Command_Core::UserLevel::Member},
            {TermType::UserQuit, Command_Core::UserLevel::Admins},
            {TermType::AdminsSet, Command_Core::UserLevel::Creator},
            {TermType::AdminsRemove, Command_Core::UserLevel::Creator},
            {TermType::AdminsList, Command_Core::UserLevel::Member},
            {TermType::UserList, Command_Core::UserLevel::Member},
            {TermType::Bind, Command_Core::UserLevel::Creator},
            {TermType::History, Command_Core::UserLevel::Member},
            {TermType::SaveToFile, Command_Core::UserLevel::Creator},
            {TermType::CleanHistory, Command_Core::UserLevel::Admins},
            {TermType::Set, Command_Core::UserLevel::Admins},
            {TermType::Export, Command_Core::UserLevel::Admins},
            {TermType::Import, Command_Core::UserLevel::Admins},
            {TermType::Help, Command_Core::UserLevel::Banned},
            {TermType::Others, Command_Core::UserLevel::Banned},
            {TermType::NULLCommand, Command_Core::UserLevel::Banned}
        };


        /**
         * @brief 获取历史记录的辅助函数
         *
         * @param term 包含参数的命令项
         * @param manager 会话管理器
         * @param context 命令上下文
         * @return 格式化后的历史记录字符串
         */
        std::string GetHistory(Sessions::SessionsCommandTemp::ExpressionTerm &term, Sessions::SessionManagerTemp::SessionManager* manager, Command_Core::ICommandContext& context)
        {
            const auto& List = manager->GetDiceSystem(context)->GetDiceEventListConst();

            std::string result;
            std::string time;


            context.SendResult(std::format("历史记录总数: {}", List.size()), Command_Core::MessageTarget::CurrentChannel);
            if(term.args.size() < 1)
            {
                //默认列出20条
                size_t count = 20;
                for(auto&& event : List)
                {
                    if(count-- == 0)
                        break;
                    
                    //time
                    time.clear();
                    time = std::format("{:%Y年%m月%d日%H时%M分%S秒}", event->GetTimeInfo());

                    result += std::format("{} 在 {} 投掷出 ", ResolvePlayerName(*event), time);
                    result += std::format("{}\n", event->ParseExpressionStr());
                    result += std::format("备注: {}\n", event->GetThrowLog().empty() ? "NULL" : event->GetThrowLog());
                }
            }
            else if(term.args.size() == 1)
            {
                auto count = std::stoi(term.args[0]);
                for(auto&& event : List)
                {
                    if(count-- == 0)
                        break;
                    
                    //time
                    time.clear();
                    time = std::format("{:%Y年%m月%d日%H时%M分%S秒}", event->GetTimeInfo());

                    result += std::format("{} 在 {} 投掷出 ", ResolvePlayerName(*event), time);
                    result += std::format("{}\n", event->ParseExpressionStr());
                    result += std::format("备注: {}\n", event->GetThrowLog().empty() ? "NULL" : event->GetThrowLog());
                }
            }
            else
            {
                return "SessionsHistory: 参数过多或过少！";
            }

            context.SendResult(std::format("历史记录获取成功:\n{}", result), Command_Core::MessageTarget::CurrentChannel);
            return result;
        }
    };
}







namespace Sessions
{
    /**
     * @brief 向命令注册表注册会话管理命令
     *
     * 插件初始化时调用此函数，将 `SessionsCommand` 处理器注册到核心命令系统中。
     *
     * @param registry 命令注册表接口
     */
    void RegisterSessionsCommand(Command_Core::ICommandRegistry& registry);
}

