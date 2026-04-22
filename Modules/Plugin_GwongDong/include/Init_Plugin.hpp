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
#include <GwongDongFileSystem.hpp>

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
    /**
     * @brief 插件元信息单例类
     *
     * 提供对插件基本信息的统一访问入口。内部采用 PIMPL 模式隐藏实现细节，
     * 并通过单例模式保证全局唯一实例。信息包括插件名称、版本、作者和描述。
     *
     * 用法：
     * @code
     * const auto& info = Plugin::PluginInfo::get_Instance();
     * auto [name, desc, author, version] = info.get(); // 结构化绑定
     * @endcode
     *
     * @note 该类为只读元信息提供者，不应包含可变状态。
     */
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

        /**
         * @brief 根据字段名获取对应的元信息字符串
         * @param str 字段标识（如 PLUGIN_NAME 宏的值）
         * @return 对应的元信息字符串，若未匹配则返回空字符串
         */
        std::string get(std::string str);

        //重命名
        using PluginInfoResSet = std::tuple<std::string,std::string,std::string,std::string>;

        /**
         * @brief 获取插件元信息元组
         * @return std::tuple<std::string, std::string, std::string, std::string>
         *         依次为：名称、描述、作者、版本
         */
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



/**
 * @brief 服务器 UUID 与连接句柄的双向映射管理器
 *
 * TeamSpeak 3 中每个虚拟服务器有唯一的 UUID，而 SDK 使用临时连接句柄（uint64）
 * 标识当前连接。该类维护两者之间的双向映射，并提供线程安全的增删查操作，
 * 用于在回调中快速通过句柄获取 UUID 或反之。
 *
 * @note 所有公共方法均受内部 std::mutex 保护，保证多线程环境下的数据一致性。
 */
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

    /**
     * @brief 建立 UUID 与连接句柄的映射关系
     * @param UUID 服务器唯一标识符
     * @param Handler 连接句柄
     * @return 始终返回 true（当前实现）
     */
    bool Build(const std::string UUID, const uint64 Handler)
    {
        std::lock_guard<std::mutex> lock(UUID_map_mutex);

        UUIDToHandlerMap.try_emplace(UUID, Handler);
        HandlerToUUIDMap.try_emplace(Handler, UUID);
        return true;
    }

    /**
     * @brief 删除 UUID 与连接句柄的映射关系
     * @param UUID 待删除的 UUID
     * @return 删除成功返回 true，否则返回 false
     */
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

    /**
     * @brief 根据 Handler 删除对应的连接句柄映射关系
     * @param Handler 待删除的 Handler 字符串
     * @return 删除成功则返回 true，否则返回 false
     */
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

    /**
     * @brief 根据连接句柄获取对应的服务器 UUID
     * @param Handler 连接句柄
     * @return 若存在映射则返回 UUID 字符串视图，否则返回空视图
     */
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

    /**
     * @brief 根据 UUID 获取对应的连接句柄
     * @param UUID 服务器 UUID
     * @return 若存在映射则返回对应的连接句柄，否则返回 0
     */
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

/**
 * @brief 信息包装器函数类型
 *
 * 定义从 TS3 原始事件参数构造 Command_Core::InfoFetcher 对象的回调签名。
 * 实现者负责调用 TS3 SDK 获取用户名、频道名、服务器名等附加信息。
 *
 * @param serverConnectionHandlerID 连接句柄
 * @param fromID 消息发送者的客户端 ID
 * @param channelID 发送者所在的频道 ID
 * @param isPrivate 是否为私聊消息
 * @return 填充完毕的 InfoFetcher 结构体
 */
using FillInfoPackageFunc = std::function<Command_Core::InfoFetcher(
    uint64 serverConnectionHandlerID,
    anyID fromID,
    uint64 ChannelID,
    bool isPrivate
)>;

/**
 * @brief 全局信息包装器实例
 *
 * 在插件初始化时被赋值为一个 lambda 表达式，用于在 onTextMessageEvent 等回调中
 * 快速构建 InfoFetcher。该实例被多处调用，是实现消息处理流程的关键适配器。
 */
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
