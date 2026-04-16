#pragma once


#include <memory>
#include <array>
#include <functional>

#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>


/**
 * @brief 日志命名空间
 *  
 * @author Sora32314
 * @date 2026/4/16 20:27
 */
namespace Plugin_Logs
{
    using logLevel = spdlog::level::level_enum;

    
    /**
     * @brief 日志类
     *  
     * 这个类封装了SPDlog库的功能，并封装了日志的初始化、记录、销毁等功能。
     * 这个类是Async的。
     * 
     * @warning 这个类不允许拷贝与赋值。
     */
    class Log
    {
    public:
        ~Log() = default;
        Log(std::string_view logPath);

        //禁止拷贝和赋值
        Log(const Log&) = delete;
        Log& operator=(const Log&) = delete;

    public:
        //成员函数————async

        /**
         * @brief 记录日志
         *  
         * @param logMSG 日志内容
         * @param lv 日志等级
         * @param nowFlush 是否立即刷新
         */
        void Logging(std::string_view logMSG, logLevel lv, bool nowFlush);

        /**
         * @brief 自行创建日志文件。
         * 
         * @param dir log文件存放的目录
         */
        void CreatLoggingFile(std::string& dir);

        /**
         * @brief 销毁日志实例
         *  
         */
        void DestroyInstance();
        //TODO:错误查询。
        //void FindError();

    private:

        /**
         * @brief 日志实现类
         * 日志实现类，使用了pImpl 模式。
         */
        class Impl;

        /**
         * @brief 日志实现类
         * 日志实现类，使用了pImpl 模式。
         */
        std::unique_ptr<Impl> pImpl;
    };

    

}








