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


//命令核心抽象
namespace Command_Core
{
    using anyID = unsigned short;

    //信息获取接口
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

    //客户端信息类型包
    //不存储ServerConnectionHandler，只要她们UUID一致，在逻辑上认为她们是同一个。
    typedef struct {
        std::string nickName;
        anyID clientID;
        uint64_t channelID;
        std::string uniqueID;
    } ClientInfoPackage;

    using CC_LogCallback = std::function<void(const std::string&, Plugin_Logs::logLevel, bool nowFlush)>;
    void SetLogCallback(CC_LogCallback callback);


    //用户实际结构体 For SessionManager
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
    //管理员结构体
    struct Admin : public User
    {
        int PermissionLevel = 0;
    };

    //消息类型目标参数
    enum class MessageTarget
    {
        PrivateMessage = 1,
        CurrentChannel,
        Server
    };

    //结果回调接口
    class IResultCallback
    {
    public:
        virtual ~IResultCallback() = default;
        virtual void SendResult
        (
            uint64 userID,
            uint64 channelID,
            uint64 serverID,
            MessageTarget target,
            const std::string& message
        ) = 0;
    };

    //命令上下文
    class ICommandContext
    {
    public:
        virtual ~ICommandContext() = default;

    public:
        virtual std::string_view GetRawCommand() const = 0;

        virtual std::string_view GetCallingUserName() const = 0;

        virtual std::string_view GetChannelName() const = 0;

        virtual std::string_view GetServerName() const = 0;

        virtual uint64 GetCallingUserID() const = 0;

        virtual uint64 GetChannelID() const = 0;

        virtual uint64 GetServerID() const = 0;

        virtual void Log(std::string_view message, Plugin_Logs::logLevel level, bool nowFlush) const = 0;

        virtual std::optional<std::string_view> GetParam(size_t index) const = 0;
    
        virtual size_t GetParamCount() const = 0;

        virtual void SendResult(const std::string& message, MessageTarget target = MessageTarget::CurrentChannel) const = 0;

        virtual MessageTarget GetMessageTarget() const = 0;

        virtual std::unordered_map<std::string, ClientInfoPackage> GetAllUsersInChannel() const = 0;

        virtual std::unordered_map<std::string, ClientInfoPackage> GetAllUsersInServer() const = 0;
    
        virtual std::unordered_map<std::string, ClientInfoPackage> GetAllUsers() const = 0;

        virtual std::vector<std::string> GetUserNameByID(uint64 userID) const = 0;

        virtual std::vector<std::string> GetUserNameByID(uint64_t serverHandler, uint64 userID) const = 0;

        virtual std::string_view GetUserUUID(uint64 userID) const = 0;

        virtual ClientInfoPackage GetUser(uint64 userID) const = 0;

        //因为昵称在服务器内唯一，所以这里只返回一个确定的结果。
        virtual ClientInfoPackage GetUserByName(std::string_view name) const = 0;
        
    };

    //命令处理器接口
    class ICommandHandler
    {
    public:
        virtual void Execute(ICommandContext& context) = 0;
        virtual std::string_view GetName() const = 0;
        virtual std::string_view GetDescription() const = 0;
        virtual std::vector<std::string_view> GetAlias() const = 0;
        virtual ~ICommandHandler() = default;
    };

    //命令注册器接口
    class ICommandRegistry
    {
    public:
        virtual ~ICommandRegistry() = default;

        virtual void RegisterHandler(std::unique_ptr<ICommandHandler> handler) = 0;
        virtual ICommandHandler* FindHandler(std::string_view command) const = 0;
    };

    //检查器
    class ICommandChecker
    {
    public:
        virtual ~ICommandChecker() = default;

        virtual void SetResultCallback(std::shared_ptr<IResultCallback> callback) = 0;

        virtual bool ProcessMessage
        (
            std::string_view message,
            InfoFetcher info_package,
            MessageTarget messageTarget
        ) = 0;
    };

    //获取客户端信息类型
    //我是蓝狗，懒得写事件总线了，等有需求再改。
    class IClientInfoStorage
    {
    public:
        virtual ~IClientInfoStorage() = default;
    public:
        virtual std::unordered_map<uint64_t, std::unordered_map<std::string, ClientInfoPackage>>& GetClientCache()  = 0;
    };

    //获取客户端信息缓存接口
    IClientInfoStorage& GetClientInfoStorage();

    //工厂函数
    std::unique_ptr<ICommandRegistry> CreateRegistry();
    std::unique_ptr<ICommandChecker> CreateCMDChecker(ICommandRegistry& registry);

}



