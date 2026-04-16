#include "../include/loggings.hpp"
#include "loggings.hpp"




namespace Plugin_Logs
{

    /**
     * @brief 日志实现类
     * 封装了SPDlog的日志实现类。
     */
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

        /**
         * @brief 日志系统安全关闭函数
         * 确保日志系统在程序退出时安全关闭。
         * 
         * @warning 请勿在程序运行过程中调用此函数。
         * @warning 请在程序退出时调用此函数用于安全退出。
         */
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

        /**
         * @brief 日志初始化函数
         * 
         * @param logPath 日志文件路径
         * 
         * @details
         * 1. 创建一个线程池，用于异步写入日志文件。
         * 2. 创建一个异步日志文件，用于记录日志信息。
         * 3. 设置日志头格式。
         * 4. 设置日志启动提示，用于Debug测试。
         */
        void Init(std::string_view logPath) noexcept
        {
            if (logPool == nullptr)
            {
                //初始化线程池容量
                spdlog::init_thread_pool(8192,1);           //默认设置8K容量以及单个工作线程运行。
                //创建线程池
                logPool = spdlog::create_async<spdlog::sinks::basic_file_sink_mt>("async_file_logger", logPath.data());
                logPool->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [t=%t] %v");
                #ifdef _DEBUG
                logPool->log(spdlog::level::info, "[Loggings Info] Loggings系统启动成功！");
                #endif
            }
        }

    public:
        std::shared_ptr<spdlog::logger> logPool;
        
        /// @brief 设置日志级别为trace
        /// @param msg 
        void log_trace(const std::string_view msg)
        {
            logPool->log(logLevel::trace, msg);
        }

        /// @brief 设置日志级别为debug
        /// @param msg 
        void log_debug(const std::string_view msg)
        {
            logPool->log(logLevel::debug, msg);
        }

        /// @brief 设置日志级别为info
        /// @param msg 
        void log_info(const std::string_view msg)
        {
            logPool->log(logLevel::info, msg);
        }

        /// @brief 设置日志级别为warn
        /// @param msg 
        void log_warn(const std::string_view msg)
        {
            logPool->log(logLevel::warn, msg);
        }

        /// @brief 设置日志级别为error
        /// @param msg 
        void log_error(const std::string_view msg)
        {
            logPool->log(logLevel::err, msg);
        }

        /// @brief 设置日志级别为critical
        /// @param msg 
        void log_critical(const std::string_view msg)
        {
            logPool->log(logLevel::critical, msg);
        }

        /// @brief 设置日志级别为off
        /// @param msg 
        void log_off(const std::string_view msg)
        {
            logPool->log(logLevel::off, msg);
        }

        /**
         * @brief 这个数组用于存储日志函数指针。
         * 
         */
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


    /// @brief 此构造函数会创建一个日志实现类对象。
    /// @param logPath 
    Log::Log(std::string_view logPath) : pImpl(std::make_unique<Log::Impl>(logPath)) {}


    /**
     * @brief 日志函数
     * 
     * @param logMSG 日志信息
     * @param lv 日志级别
     * @param nowFlush 是否立即刷新日志
     * 
     * @details
     * 1. 根据日志级别选择对应的日志函数。
     * 2. 调用日志函数进行日志记录。
     * 3. 如果nowFlush为true，则立即刷新日志。
     */
    void Log::Logging(std::string_view logMSG, logLevel lv, bool nowFlush)
    {
        auto funcLog = pImpl->log_functions.at(lv);
        (pImpl.get()->*funcLog)(logMSG);

        if(nowFlush)
        {
            (pImpl.get()->logPool)->flush();
        }
    }

    /// @brief 销毁日志实现类对象
    void Log::DestroyInstance()
    {
        pImpl->SafeShutdown();
    }
}




