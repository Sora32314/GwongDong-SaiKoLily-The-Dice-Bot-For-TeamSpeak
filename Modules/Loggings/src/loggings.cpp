#include "../include/loggings.hpp"
#include "loggings.hpp"




namespace Plugin_Logs
{

    class Log::Impl
    {
    public:
        using LogFunction = void (Impl::*)(const std::string_view);
        
        
    public:
        Impl(std::string_view logPath)
        {
            Init(logPath);
        }
        ~Impl() = default;

        void SafeShutdown()
        {
            if(logPool) {
                logPool->info("安全关闭日志系统...");
                logPool->flush();
                spdlog::drop("async_file_logger");
                logPool.reset();
            }
        }

        //禁止拷贝和复制
        Impl(const Impl&) = delete;
        Impl& operator=(const Impl&) = delete;

        //spdlog封装类log初始化函数
        void Init(std::string_view logPath) noexcept
        {
            if (logPool == nullptr)
            {
                //初始化线程池容量
                spdlog::init_thread_pool(8192,1);           //默认设置8K容量以及单个工作线程运行。
                //创建线程池
                logPool = spdlog::create_async<spdlog::sinks::basic_file_sink_mt>("async_file_logger", logPath.data());
                logPool->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [t=%t] %v");
                logPool->log(spdlog::level::info, "[Loggings Info] Aloha~!");
            }
        }

    public:
        std::shared_ptr<spdlog::logger> logPool;
        
        void log_trace(const std::string_view msg)
        {
            logPool->log(logLevel::trace, msg);
        }
        void log_debug(const std::string_view msg)
        {
            logPool->log(logLevel::debug, msg);
        }
        void log_info(const std::string_view msg)
        {
            logPool->log(logLevel::info, msg);
        }
        void log_warn(const std::string_view msg)
        {
            logPool->log(logLevel::warn, msg);
        }
        void log_error(const std::string_view msg)
        {
            logPool->log(logLevel::err, msg);
        }
        void log_critical(const std::string_view msg)
        {
            logPool->log(logLevel::critical, msg);
        }
        void log_off(const std::string_view msg)
        {
            logPool->log(logLevel::off, msg);
        }

        //log函数表
        std::array<LogFunction, logLevel::n_levels> log_functions
        {
            &Impl::log_trace,
            &Impl::log_debug,
            &Impl::log_info,
            &Impl::log_warn,
            &Impl::log_error,
            &Impl::log_critical,
            &Impl::log_off
        };



    };

    //============Log类实现================

    Log::Log(std::string_view logPath) : pImpl(std::make_unique<Log::Impl>(logPath)) {}

    void Log::Logging(std::string_view logMSG, logLevel lv, bool nowFlush)
    {
        auto funcLog = pImpl->log_functions.at(lv);
        (pImpl.get()->*funcLog)(logMSG);

        if(nowFlush)
        {
            (pImpl.get()->logPool)->flush();
        }
    }
    
    void Log::DestroyInstance()
    {
        pImpl->SafeShutdown();
    }
}




