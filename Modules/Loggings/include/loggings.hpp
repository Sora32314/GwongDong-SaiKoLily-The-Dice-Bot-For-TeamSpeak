#pragma once


#include <memory>
#include <array>
#include <functional>

#include <spdlog/spdlog.h>
#include <spdlog/logger.h>
#include <spdlog/sinks/base_sink.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/async.h>

namespace Plugin_Logs
{
    using logLevel = spdlog::level::level_enum;

    // spdlog 日志库简单封装
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
        void Logging(std::string_view logMSG, logLevel lv, bool nowFlush);
        void CreatLoggingFile(std::string& dir);
        void DestroyInstance();
        //TODO:错误查询。
        //void FindError();

    private:
        class Impl;
        std::unique_ptr<Impl> pImpl;
    };

    

}








