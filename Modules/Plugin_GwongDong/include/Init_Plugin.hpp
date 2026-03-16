#pragma once

#ifdef _WIN32_

#include <Windows.h>

#endif

//基础头文件
#include <iostream>

#include <tuple>

#include <string>
#include <functional>
#include <string_view>
#include <memory>

//TS3头文件
#include "../../../ExternModules/ts3client-pluginsdk-26/include/teamspeak/public_definitions.h"
#include "../../../ExternModules/ts3client-pluginsdk-26/include/teamspeak/public_errors.h"
#include "../../../ExternModules/ts3client-pluginsdk-26/include/teamspeak/public_errors_rare.h"
#include "../../../ExternModules/ts3client-pluginsdk-26/include/teamspeak/public_rare_definitions.h"
#include "../../../ExternModules/ts3client-pluginsdk-26/include/ts3_functions.h"

//自定义库
#include <loggings.hpp>
#include <CommandCore.hpp>
#include <SaiKoLily.hpp>
#include <CMD_SaiKoLily.hpp>
#include <SessionManager.hpp>

//添加导出到库的标识
#ifndef LIB_EXPORT
    #define LIB_EXPORT
#endif


// ______ _             _             _____      _ _   
// | ___ \ |           (_)           |_   _|    (_) |  
// | |_/ / |_   _  __ _ _ _ __  ___    | | _ __  _| |_ 
// |  __/| | | | |/ _` | | '_ \/ __|   | || '_ \| | __|
// | |   | | |_| | (_| | | | | \__ \  _| || | | | | |_ 
// \_|   |_|\__,_|\__, |_|_| |_|___/  \___/_| |_|_|\__|
//                 __/ |                               
//                |___/                                                                                                                                       
//============插件初始化============



//============基础信息============

namespace Plugin
{
    class LIB_EXPORT PluginInfo
    {
    private:
        PluginInfo();

    public:
        ~PluginInfo();
        
        //单例模式全局只允许存在一个元数据实例
        static auto get_Instance()
        -> PluginInfo&
        {
            static PluginInfo instance;
            return instance;
        }

        std::string get(std::string str);

        //重命名
        using PluginInfoResSet = std::tuple<std::string,std::string,std::string,std::string>;
        PluginInfoResSet get();
        
        //禁止拷贝和赋值
        PluginInfo(const PluginInfo&) = delete;
        PluginInfo& operator=(const PluginInfo &) = delete;

        

    private:

        //内部类实现
        class Impl;
        std::unique_ptr<Impl> pImpl;
        
    };



}


//服务器UUID到连接句柄以及其反向映射。
class ServerUUID final
{
public:
    ServerUUID(std::unordered_map<std::string, uint64>& _UUIDToHandlerMap, std::unordered_map<uint64, std::string>& _HandlerToUUIDMap) :
    UUIDToHandlerMap(_UUIDToHandlerMap),
    HandlerToUUIDMap(_HandlerToUUIDMap)
    {}

    ServerUUID() {}

    ~ServerUUID() = default;

public:

    bool Build(const std::string UUID, const uint64 Handler)
    {
        std::lock_guard<std::mutex> lock(UUID_map_mutex);

        UUIDToHandlerMap.try_emplace(UUID, Handler);
        HandlerToUUIDMap.try_emplace(Handler, UUID);
        return true;
    }

    bool DeleteByUUID(const std::string& UUID)
    {
        std::lock_guard<std::mutex> lock(UUID_map_mutex);

        auto cache = UUIDToHandlerMap.find(UUID);

        if (cache == UUIDToHandlerMap.end())
        {
            return false;
        }

        uint64 handler = cache->second;

        HandlerToUUIDMap.erase(handler);

        UUIDToHandlerMap.erase(cache);

        return true;
    }

    bool DeleteByHandler(const uint64& Handler)
    {
        std::lock_guard<std::mutex> lock(UUID_map_mutex);

        auto cache = HandlerToUUIDMap.find(Handler);

        if (cache == HandlerToUUIDMap.end())
        {
            return false;
        }

        auto UUID = cache->second;

        UUIDToHandlerMap.erase(UUID);

        HandlerToUUIDMap.erase(cache);

        return true;
    }

    std::string_view GetUUID(uint64 Handler)
    {
        std::lock_guard<std::mutex> lock(UUID_map_mutex);

        auto it = HandlerToUUIDMap.find(Handler);
        if (it != HandlerToUUIDMap.end())
        {
            return it->second;
        }
        return "";
    }

    uint64 GetConnectingHandler(std::string_view UUID)
    {
        std::lock_guard<std::mutex> lock(UUID_map_mutex);

        auto it = UUIDToHandlerMap.find(std::string(UUID));
        if (it != UUIDToHandlerMap.end()) {
            return it->second;
        }
        return 0;
    }

private:
    std::mutex UUID_map_mutex;
    std::unordered_map<std::string, uint64> UUIDToHandlerMap;
    std::unordered_map<uint64, std::string> HandlerToUUIDMap;
};

//填充用户信息包
using FillInfoPackageFunc = std::function<Command_Core::InfoFetcher(
    uint64 serverConnectionHandlerID,
    anyID fromID,
    uint64 ChannelID,
    bool isPrivate
)>;
extern FillInfoPackageFunc FillInfoPackage;


extern "C"
{
    //插件基本信息
    LIB_EXPORT const char* ts3plugin_name();
    
    LIB_EXPORT const char* ts3plugin_description();

    LIB_EXPORT const char* ts3plugin_author();

    LIB_EXPORT const char* ts3plugin_version();

    LIB_EXPORT void ts3plugin_setFunctionPointers(const struct TS3Functions funcs);

    //插件API版本
    LIB_EXPORT int ts3plugin_apiVersion();

    //插件初始化执行代码
    LIB_EXPORT int ts3plugin_init();

    //插件退出执行代码
    LIB_EXPORT void ts3plugin_shutdown();

    //弃用
    //创建新的服务器连接句柄
    //LIB_EXPORT unsigned int (*spawnNewServerConnectionHandler)(int port, uint64* result);
    //移除服务器连接句柄
    //LIB_EXPORT unsigned int (*destroyServerConnectionHandler)(uint64 serverConnectionHandlerID);
    //开始连接服务器
    //LIB_EXPORT unsigned int (*startConnection)(uint64 serverConnectionHandlerID, const char* identity, const char* ip, unsigned int port, const char* nickname, const char** defaultChannelArray, const char* defaultChannelPassword, const char* serverPassword);






    
    //  _____       _ _ _                _     ______                _   _             
    // /  __ \     | | | |              | |    |  ___|              | | (_)            
    // | /  \/ __ _| | | |__   __ _  ___| | __ | |_ _   _ _ __   ___| |_ _  ___  _ __  
    // | |    / _` | | | '_ \ / _` |/ __| |/ / |  _| | | | '_ \ / __| __| |/ _ \| '_ \ 
    // | \__/\ (_| | | | |_) | (_| | (__|   <  | | | |_| | | | | (__| |_| | (_) | | | |
    //  \____/\__,_|_|_|_.__/ \__,_|\___|_|\_\ \_|  \__,_|_| |_|\___|\__|_|\___/|_| |_|
    //                                                                                 
    //============回调函数============                                                  

    //连接状态改变                              回调函数
    LIB_EXPORT void ts3plugin_onConnectStatusChangeEvent(uint64 serverConnectionHandlerID, int newStatus, int errorNumber);

    //接收信息                                 回调函数
    LIB_EXPORT int ts3plugin_onTextMessageEvent(uint64 serverConnectionHandlerID, anyID targetMode, anyID toID, anyID fromID, const char* fromName, const char* fromUniqueIdentifier, const char* message, int ffIgnored);

    //客户端加入/切换频道                       回调函数
    LIB_EXPORT void ts3plugin_onClientMoveEvent(uint64 serverConnectionHandlerID, anyID clientID, anyID oldChannelID, uint64 newChannelID, int visibility, const char* moveMessage);

    //客户端被移动                             回调函数
    LIB_EXPORT void ts3plugin_onClientMoveMovedEvent(uint64 serverConnectionHandlerID, anyID clientID, anyID oldChannelID, uint64 newChannelID, anyID moverID, const char* moverName, const char* moverUniqueIdentifier, int visibility, const char* moveMessage);

    //客户端断开连接                            回调函数
    LIB_EXPORT void ts3plugin_onClientDisconnected(uint64 serverConnectionHandlerID, anyID clientID, const char* disconnectMsg);

    //昵称变更                                 回调函数
    LIB_EXPORT void ts3plugin_onClientDisplayNameChanged(uint64 serverConnectionHandlerID, anyID clientID, const char* newDisplayName, const char* uniqueClientIdentifier);

    //错误处理                                 回调函数
    LIB_EXPORT int ts3plugin_onServerErrorEvent(uint64 serverConnectionHandlerID, const char* errorMessage, unsigned int error, const char* returnCode, const char* extraMessage);

    //当所有频道订阅成功后                      回调函数
    LIB_EXPORT void ts3plugin_onChannelSubscribeFinishedEvent(uint64 serverConnectionHandlerID);


}
