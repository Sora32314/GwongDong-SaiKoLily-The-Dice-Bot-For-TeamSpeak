#include <windows.h>

#include "../include/Init_Plugin.hpp"
#include "Init_Plugin.hpp"


#define PLUGIN_NAME                                         "GwongDong SaiKoLily(骰娘小广东)"
#define PLUGIN_DESCRIPTION                                  "TeamSpeak 骰娘BOT\"小广东\"。"
#define PLUGIN_AUTHOR                                       "Sora32314"
#define PLUGIN_VERSION                                      "0.0.3 Insider Test Useable Version."

#define PLUGIN_API_VERSION                                  26

#define PATH_BUFSIZE 512
const char* pluginID = "114514";

//TS3Function库
static struct TS3Functions ts3Functions;

auto logInstance = new Plugin_Logs::Log("logs/async_logs.txt");

const char* Identity = "h9VyNroBRfOChMtxJXv+kennJhg=";

uint64 serverConnectionHandlerID;

std::shared_ptr<SaiKoLily::DiceSystem::DiceSystemImpl> DS = std::make_shared<SaiKoLily::DiceSystem::DiceSystemImpl>();

static auto registry = Command_Core::CreateRegistry();
static auto checker = Command_Core::CreateCMDChecker(*registry);

//获取User缓存
auto& clientCache = Command_Core::GetClientInfoStorage();



//暂时构建UUID映射方法
static ServerUUID ServerUUIDCache;


std::mutex clientCacheMutex;

namespace Plugin
{

    class PluginInfo::Impl
    {
    private:
        Impl() : name(PLUGIN_NAME), description(PLUGIN_DESCRIPTION), author(PLUGIN_AUTHOR), version(PLUGIN_VERSION) {}
        
    public:
        ~Impl() = default;

        static auto get_Instance()
        -> Impl&
        {
            static Impl instance;
            return instance;
        }

        std::string name;
        std::string version;
        std::string author;
        std::string description;
    };

    //============PluginInfo类实现================

    PluginInfo::PluginInfo()
    {
        const auto& instance = Impl::get_Instance();
        pImpl = std::make_unique<Impl>(instance);
    }

    PluginInfo::~PluginInfo() = default;

    PluginInfo::PluginInfoResSet PluginInfo::get()
    {
        auto resSet = std::make_tuple(PLUGIN_NAME, PLUGIN_DESCRIPTION, PLUGIN_AUTHOR, PLUGIN_VERSION);
        return resSet;
    }

    std::string PluginInfo::get(std::string input)
    {

        if(input.c_str() == PLUGIN_NAME)
        {
            return pImpl->name;
        }
        else if(input.c_str() == PLUGIN_DESCRIPTION)
        {
            return pImpl->description;
        }
        else if(input.c_str() == PLUGIN_AUTHOR)
        {
            return pImpl->author;
        }
        else if(input.c_str() == PLUGIN_VERSION)
        {
            return pImpl->version;
        }
        
        return std::string();
    }

}

//安全字符串包装
class SafeString
{
public:
    char* ptr = nullptr;
    ~SafeString() { if (ptr) ts3Functions.freeMemory(ptr); }
    operator std::string() const 
    {
        return ptr ? ptr : ""; 
    }
};

// void UpdateServerUserCache(uint64 serverConnectionHandlerID)
// {
//     std::lock_guard lock(clientCacheMutex);
//     Command_Core::ClientInfoPackage.erase(serverConnectionHandlerID);
//     anyID* clientIDArray = nullptr;
//     if(ts3Functions.getClientList(serverConnectionHandlerID, &clientIDArray) == ERROR_ok && clientIDArray)
//     {
//         for(size_t i = 0; clientIDArray[i] != 0; i++)
//         {
//             anyID clientID = clientIDArray[i];
//             char* clientName = nullptr;
//             if(ts3Functions.getClientVariableAsString(serverConnectionHandlerID, clientID, CLIENT_NICKNAME, &clientName) == ERROR_ok && clientName)
//             {
//                 Command_Core::ClientInfoPackage[serverConnectionHandlerID].push_back({std::string(clientName), clientID});
//                 ts3Functions.freeMemory(clientName);
//             }
//         }
//         ts3Functions.freeMemory(clientIDArray);
//     }
// }

//信息包装器 匿名函数包装
FillInfoPackageFunc FillInfoPackage = [](uint64 serverConnectionHandlerID,
                                        anyID fromID,
                                        uint64 channelID,
                                        bool isPrivate) -> Command_Core::InfoFetcher {
    Command_Core::InfoFetcher info;//{0, 0, 0, "", std::nullopt, std::nullopt, std::nullopt, Command_Core::ClientInfoPackage};
    
    // 填充基本ID信息
    info.userID = fromID;
    info.channelID = channelID;
    info.serverID = serverConnectionHandlerID;
    
    // 获取服务器UUID
    if (auto serverUUID = ServerUUIDCache.GetUUID(serverConnectionHandlerID); !serverUUID.empty()) {
        info.serverUUID = std::string(serverUUID);
    }
    
    // 获取用户名
    SafeString userName;
    if (ts3Functions.getClientVariableAsString(serverConnectionHandlerID, fromID, CLIENT_NICKNAME, &userName.ptr) == ERROR_ok) {
        info.UserName = static_cast<std::string>(userName);
    }
    
    // 获取服务器名称
    SafeString serverName;
    if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_NAME, &serverName.ptr) == ERROR_ok) {
        info.ServerName = static_cast<std::string>(serverName);
    }
    
    // 私聊，不需要频道名称直接返回
    if (isPrivate) {
        return info;
    }
    
    // 非私聊时获取频道名称
    SafeString channelName;
    if (ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, channelID, CHANNEL_NAME, &channelName.ptr) == ERROR_ok) {
        info.ChannelName = static_cast<std::string>(channelName);
    }
    
    return info;
};




//信息发送回调
class TS3ResultCallback : public Command_Core::IResultCallback
{
public:
    void SendResult(
        uint64 userID,
        uint64 channelID,
        uint64 serverID,
        Command_Core::MessageTarget target,
        const std::string& message
    ) override {
        // 根据目标类型发送消息
        switch (target) {
            case Command_Core::MessageTarget::PrivateMessage:
                // 发送私聊消息
                ts3Functions.requestSendPrivateTextMsg(serverID, message.c_str(), userID, nullptr);
                break;
            case Command_Core::MessageTarget::CurrentChannel:
                // 发送到当前频道
                ts3Functions.requestSendChannelTextMsg(serverID, message.c_str(), channelID, nullptr);
                break;
            case Command_Core::MessageTarget::Server:
                // 发送到服务器
                ts3Functions.requestSendServerTextMsg(serverID, message.c_str(), nullptr);
                break;
        }
    }
};

static auto resultCallback = std::make_shared<TS3ResultCallback>();

//获取所有频道的用户信息

int ts3plugin_onServerErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, const char* extraMessage)
{
    logInstance->Logging(std::format("进入处理！错误码：{}", std::string(returnCode)), Plugin_Logs::logLevel::info, false);

    if(error == ERROR_ok)
    {
        logInstance->Logging(std::format("订阅所有频道成功！"), Plugin_Logs::logLevel::info, false);
        return 1;
    }

    return 0;
}

void ts3plugin_onChannelSubscribeFinishedEvent(uint64 serverConnectionHandlerID)
{
    logInstance->Logging(std::format("订阅所有频道成功！"), Plugin_Logs::logLevel::info, false);

    anyID* clientList = nullptr;
    uint64* channelList = nullptr;

    if(ts3Functions.getChannelList(serverConnectionHandlerID, &channelList) != ERROR_ok)
    {
        logInstance->Logging("获取频道列表失败！", Plugin_Logs::logLevel::err, false);
        return;
    }

    if(ts3Functions.getClientList(serverConnectionHandlerID, &clientList) != ERROR_ok) {
        return;
    }
    
    for(uint64 *p = channelList; *p != 0; p++)
    {
        uint64 channelID = *p;

        anyID* channelClientList = nullptr;
        if(ts3Functions.getChannelClientList(serverConnectionHandlerID, channelID, &channelClientList) == ERROR_ok && channelClientList)
        {
            
            for(anyID *p = channelClientList; *p != 0; p++)
            {
                Command_Core::ClientInfoPackage clientInfo;
                char* uid_ptr = nullptr;
                char* nickname_ptr = nullptr;

                if(ts3Functions.getClientVariableAsString(serverConnectionHandlerID, *p, CLIENT_UNIQUE_IDENTIFIER, &uid_ptr) != ERROR_ok) {
                    logInstance->Logging(std::format("获取ID:{}的UID失败！无法将ID为{}的用户存入用户集合中。", *p, *p), Plugin_Logs::logLevel::err, false);
                    continue;
                }

                if(ts3Functions.getClientVariableAsString(serverConnectionHandlerID, *p, CLIENT_NICKNAME, &nickname_ptr) != ERROR_ok) {
                    logInstance->Logging(std::format("获取UID:{}的用户名失败！", *uid_ptr), Plugin_Logs::logLevel::warn, false);
                }
                std::string nickname = nickname_ptr;
                std::string uid = uid_ptr;
                anyID clientID = *p;
                logInstance->Logging(std::format("获取频道ID为{}\t用户ID为{}\t用户名:{}\tUUID:{},在服务器句柄:{}", channelID, *p, nickname, uid, serverConnectionHandlerID), Plugin_Logs::logLevel::info, false);

                clientInfo.channelID = channelID;
                clientInfo.clientID = clientID;
                clientInfo.nickName = nickname;
                clientInfo.uniqueID = uid;

                ts3Functions.freeMemory(nickname_ptr);
                ts3Functions.freeMemory(uid_ptr);

                clientCache.GetClientCache()[serverConnectionHandlerID].try_emplace(uid, clientInfo);
            }
        }

        ts3Functions.freeMemory(channelClientList);
    }

    ts3Functions.freeMemory(channelList);
    ts3Functions.freeMemory(clientList);

    logInstance->Logging(std::format("连接句柄为：\"{}\"的所有频道用户信息缓存操作已结束！", serverConnectionHandlerID), Plugin_Logs::logLevel::info, false);
}

void preloadAllClient(uint64 serverConnectionHandlerID)
{
    char returnCode[512];  
    ts3Functions.createReturnCode(pluginID, returnCode, 64);

    auto js = ts3Functions.requestChannelSubscribeAll(serverConnectionHandlerID, returnCode);

    if (js != ERROR_ok) {  
        char* errorMsg;  
        ts3Functions.getErrorMessage(js, &errorMsg);  
        logInstance->Logging(std::format("订阅所有频道失败！错误码：{}", std::string(returnCode)), Plugin_Logs::logLevel::err, false);
        ts3Functions.freeMemory(errorMsg);  
    }
}

const char* ts3plugin_name()
{
    return PLUGIN_NAME;
}

const char* ts3plugin_description()
{
    return PLUGIN_DESCRIPTION;
}

const char* ts3plugin_author()
{
    return PLUGIN_AUTHOR;
}

int ts3plugin_apiVersion()
{
    return PLUGIN_API_VERSION;
}

const char* ts3plugin_version()
{
    return PLUGIN_VERSION;
}

void ts3plugin_setFunctionPointers(const struct TS3Functions funcs)
{
    ts3Functions = funcs;
}

int ts3plugin_init()
{
    
    logInstance->Logging("插件已加载！", Plugin_Logs::logLevel::info, false);


    ts3Functions.guiConnect(
        PLUGIN_CONNECT_TAB_NEW_IF_CURRENT_CONNECTED, 
        "Unknown", 
        "localhost", 
        "", 
        "Gwong Dong SaiKoLily", 
        "", 
        "", 
        "", 
        "", 
        "", 
        "", 
        Identity,
        "",
        "Guang Dong Sai Ko Li Li",
        &serverConnectionHandlerID
    );
    
    //设置回调函数
    SaiKoLily::SetLogCallback([](const std::string& msg, Plugin_Logs::logLevel level, bool flush)
    {
        logInstance->Logging(msg, level, flush);
    });

    Command_Core::SetLogCallback([](const std::string& msg, Plugin_Logs::logLevel level, bool flush)
    {
        logInstance->Logging(msg, level, flush);
    });

    SaiKoLily::DiceCommand::SetLogCallback([](const std::string& msg, Plugin_Logs::logLevel level, bool flush)
    {
        logInstance->Logging(msg, level, flush); 
    });

    Sessions::SetLogCallback([](const std::string& msg, Plugin_Logs::logLevel level, bool flush)
    {
       logInstance->Logging(msg, level, flush);
    });

    SaiKoLily::CheckCommand::SetLogCallback([](const std::string& msg, Plugin_Logs::logLevel level, bool flush)
    {
       logInstance->Logging(msg, level, flush);
    });

    SaiKoLily::DiceCommand::RegisterDiceCommands(*registry);
    SaiKoLily::CheckCommand::RegisterHistoryCommands(*registry);
    Sessions::RegisterSessionsCommand(*registry);

    checker->SetResultCallback(resultCallback);
    
    
    //获取频道内容
    uint64* channelIDs = nullptr;

    std::vector<std::string> channelNAME;

    if (ts3Functions.getChannelList(serverConnectionHandlerID, &channelIDs) == ERROR_ok) 
    {
        for (size_t i = 0; channelIDs[i] != 0; ++i) { // 遇到0终止遍历
            uint64 channelID = channelIDs[i];
            SafeString channelName;
            if (ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, channelID, CHANNEL_NAME, &channelName.ptr) == ERROR_ok) 
            {
                channelNAME.push_back(channelName);
            }
        }
    }

    
    // auto&& i = 0;
    // for(auto&& name : channelNAME)
    // {
    //     logInstance->Logging(std::format("频道ID:{}，频道名：{}", channelIDs[i], name), Plugin_Logs::logLevel::info);
    // }

    // 释放频道列表内存
    ts3Functions.freeMemory(channelIDs);

    return 0;
}

void ts3plugin_shutdown()
{
    //插件结束时运行代码

    spdlog::shutdown();

    logInstance->DestroyInstance();
    free(logInstance);
    logInstance = nullptr;

    ts3Functions.destroyServerConnectionHandler(serverConnectionHandlerID);

    if (pluginID) 
    {
        pluginID = NULL;
    }
}

void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, int errorNumber)
{

    logInstance->Logging(std::format("回调触发! 状态: {}", newStatus), Plugin_Logs::logLevel::debug, false);

    switch (newStatus) 
    {
        case STATUS_CONNECTING:
            logInstance->Logging("正在连接服务器...", Plugin_Logs::logLevel::info, false);
            break;

        case STATUS_CONNECTION_ESTABLISHING: 
        {
            SafeString nickname, servername;
            
            // 获取客户端ID
            anyID clientID;
            if (ts3Functions.getClientID(serverConnectionHandlerID, &clientID) != ERROR_ok) 
            {
                logInstance->Logging("获取客户端ID失败", Plugin_Logs::logLevel::err, false);
                return;
            }

            // 获取客户端昵称
            if (ts3Functions.getClientSelfVariableAsString(serverConnectionHandlerID, CLIENT_NICKNAME, &nickname.ptr) != ERROR_ok) 
            {
                logInstance->Logging("获取用户昵称失败", Plugin_Logs::logLevel::err, false);
            }

            // 获取服务器名称
            if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, VIRTUALSERVER_NAME, &servername.ptr) != ERROR_ok) 
            {
                logInstance->Logging("获取服务器名称失败", Plugin_Logs::logLevel::err, false);
            }

            // 记录日志
            logInstance->Logging(std::format("正在加入服务器: {}, 身份: {}", 
                                static_cast<std::string>(servername), 
                                static_cast<std::string>(nickname)), 
                                Plugin_Logs::logLevel::info, false);
            break;
        }

        case STATUS_CONNECTION_ESTABLISHED: 
        {
            logInstance->Logging("成功加入服务器！", Plugin_Logs::logLevel::info, false);

            //服务器信息存储
            SafeString server_name;
            SafeString server_UUID;

            // 获取频道列表
            uint64* channelIDs = nullptr;
            if (ts3Functions.getChannelList(serverConnectionHandlerID, &channelIDs) == ERROR_ok) 
            {
                std::string channelInfo = "服务器频道列表:\n";
                
                for (size_t i = 0; channelIDs[i] != 0; ++i) 
                {
                    SafeString channelName;
                    uint64 channelID = channelIDs[i];
                    
                    // 获取频道名称
                    if (ts3Functions.getChannelVariableAsString(serverConnectionHandlerID, 
                                                                channelID, 
                                                                CHANNEL_NAME, 
                                                                &channelName.ptr) == ERROR_ok) 
                    {
                        channelInfo += std::format("频道ID: {} - 名称: {}\n", channelID, static_cast<std::string>(channelName));
                    }
                }
                
                // 释放频道ID数组内存
                ts3Functions.freeMemory(channelIDs);
                
                // 记录频道信息
                //logInstance->Logging(channelInfo, Plugin_Logs::logLevel::info, false);
                
                // 发送到当前频道（仅限调试，正式环境可能需要移除）
                // ts3Functions.requestSendChannelTextMsg(serverConnectionHandlerID, 
                //                                       channelInfo.c_str(), 
                //                                       0,  // 当前频道
                //                                       nullptr);
            }

            //构建服务器UUID以及连接句柄的双向映射
            if (ts3Functions.getServerVariableAsString(serverConnectionHandlerID, 
                                                      VIRTUALSERVER_UNIQUE_IDENTIFIER, 
                                                      &server_UUID.ptr) == ERROR_ok &&
                ts3Functions.getServerVariableAsString(serverConnectionHandlerID, 
                                                      VIRTUALSERVER_NAME, 
                                                      &server_name.ptr) == ERROR_ok)
            {
                ServerUUIDCache.Build(server_UUID, serverConnectionHandlerID);
            }

            
            ts3Functions.requestSendClientQueryCommand(serverConnectionHandlerID, "usemod 4", nullptr);

            //输出客户端信息
            preloadAllClient(serverConnectionHandlerID);

            break;
        }

        case STATUS_DISCONNECTED: 
        {
            SafeString errMsg;
            if (errorNumber != ERROR_ok) 
            {
                ts3Functions.getErrorMessage(errorNumber, &errMsg.ptr);
            }
            
            std::string disconnectMsg = std::format("与服务器断开连接：{}", 
                                                    errMsg.ptr ? std::format(", 错误: {}", static_cast<std::string>(errMsg)) : "");
            
            logInstance->Logging(disconnectMsg, Plugin_Logs::logLevel::warn, false);
            
            // 销毁服务器UUID以及连接句柄的双向映射
            ServerUUIDCache.DeleteByHandler(serverConnectionHandlerID);

            // 销毁服务器连接句柄
            ts3Functions.destroyServerConnectionHandler(serverConnectionHandlerID);
            break;
        }

        default:
            logInstance->Logging(std::format("未知连接状态: {}", newStatus), 
                                Plugin_Logs::logLevel::debug, false);
    }
}

//接受信息
int ts3plugin_onTextMessageEvent(uint64 serverConnectionHandlerID, anyID targetMode, anyID toID, anyID fromID, const char* fromName, const char* fromUniqueIdentifier, const char* message, int ffIgnored)
{
    Command_Core::MessageTarget target;
    switch(targetMode)
    {
    case 1:
        target = Command_Core::MessageTarget::PrivateMessage;
        break;
    case 2:
        target = Command_Core::MessageTarget::CurrentChannel;
        break;
    case 3:
        target = Command_Core::MessageTarget::Server;
        break;
    default:
        target = Command_Core::MessageTarget::CurrentChannel;
        break;
    }

    // 1. 获取自己的客户端ID（避免处理自己发送的消息）
    anyID selfID = 0;
    if (ts3Functions.getClientID(serverConnectionHandlerID, &selfID) != ERROR_ok) {
        logInstance->Logging("获取自身客户端ID失败", Plugin_Logs::logLevel::err, false);
        return 0;
    }
    
    // 2. 忽略自己发送的消息
    if (fromID == selfID) {
        return 0;
    }
    
    // 3. 获取发送者所在的频道ID
    uint64 channelID = 0;
    if (ts3Functions.getClientVariableAsUInt64(serverConnectionHandlerID, fromID, CLIENT_CHANNEL_GROUP_ID, &channelID) != ERROR_ok) {
        logInstance->Logging("获取客户端频道ID失败", Plugin_Logs::logLevel::err, false);
        return 0;
    }
    
    // 4. 确定消息是否为私聊
    bool isPrivate = (targetMode == TextMessageTarget_CLIENT);
    if(isPrivate)
    {
        logInstance->Logging("是私聊！", Plugin_Logs::logLevel::info, false);
    }
    
    //5.构建用户信息包
    auto info_package = FillInfoPackage(
        serverConnectionHandlerID,
        fromID,
        channelID,
        isPrivate
    );


    // 6. 处理消息（如果已初始化命令检查器）
    if (checker) {
        checker->ProcessMessage(
            message,                                    // 消息内容
            info_package,                               // 用户信息包
            target                                      // 目标消息类型
        );
    } else {
        logInstance->Logging("命令检查器未初始化，无法处理消息", Plugin_Logs::logLevel::warn, false);
    }

    return 0;
}


LIB_EXPORT void ts3plugin_onClientMoveEvent(uint64 serverConnectionHandlerID, anyID clientID, anyID oldChannelID, uint64 newChannelID, int visibility, const char *moveMessage)
{
    //leaving
    logInstance->Logging(std::format("移动信息：{}", moveMessage), Plugin_Logs::logLevel::info, false);

    //退出也再move中

    for(auto&& [uid, info] : clientCache.GetClientCache()[serverConnectionHandlerID])
    {
        if(info.clientID == clientID)
        {
            {
                std::lock_guard<std::mutex> lock(clientCacheMutex);
                info.channelID = newChannelID;
            }

            logInstance->Logging(std::format("用户\"{}\"从频道{}移动到频道{}。", info.nickName, oldChannelID, info.channelID), Plugin_Logs::logLevel::info, false);

            return;
        }
    }

}

void ts3plugin_onClientMoveMovedEvent(uint64 serverConnectionHandlerID, anyID clientID, anyID oldChannelID, uint64 newChannelID, int visibility, anyID moverID, const char* moverName, const char* moverUniqueIdentifier, const char* moveMessage)
{
    auto mover_nickname = clientCache.GetClientCache()[serverConnectionHandlerID].at(moverUniqueIdentifier).nickName;

    for(auto&& [uid, info] : clientCache.GetClientCache()[serverConnectionHandlerID])
    {
        if(info.clientID == clientID)
        {
            {
                std::lock_guard<std::mutex> lock(clientCacheMutex);
                info.channelID = newChannelID;
            }

            logInstance->Logging(std::format("用户\"{}\"被用户\"{}\"从频道{}移动到频道{}。", info.nickName, std::string(std::move(mover_nickname)),oldChannelID, info.channelID), Plugin_Logs::logLevel::info, false);

            return;
        }
    }
}


