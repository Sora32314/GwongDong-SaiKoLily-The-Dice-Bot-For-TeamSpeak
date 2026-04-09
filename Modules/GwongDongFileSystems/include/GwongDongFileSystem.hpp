#pragma once

#include <ctime>
#include <random>
#include <chrono>
#include <iomanip>
#include <fstream>
//#include <expected>
#include <functional>
#include <filesystem>
#include <loggings.hpp>

namespace fs = std::filesystem;
using Path = std::filesystem::path;


namespace GwongDongFileSystem
{
    //日志记录
    using LogCallBack = std::function<void(const std::string&, Plugin_Logs::logLevel, bool nowFlush)>;

    extern LogCallBack FSLogCallback;
    void SetLogCallback(LogCallBack callback);    

    //基础文件接口(包含MetaData接口和文件操作接口)
    //拆解文件接口
    class IFileInfo
    {
    public:
        virtual ~IFileInfo() = default;

        virtual size_t GetSize() const = 0;
        virtual std::string GetFileName() const = 0;
        virtual std::string GetPathStr() const = 0;
        virtual std::tm GetFileCreateTime() const = 0;
        virtual std::tm GetFileModifyTime() const = 0;
        virtual bool Exists() const noexcept = 0;
        virtual bool IsDirectory() const = 0;
        virtual bool IsRegularFile() const = 0;
    };

    class IFileIO
    {
    private:
        mutable std::mutex _mutex;

    public:
        virtual ~IFileIO() = default;

        virtual void WriteFile(std::string_view content) const = 0;
        virtual void WriteFileMetaData(std::string_view content) const = 0;
        virtual void WriteFileFromHead(std::string_view content) const = 0;
        virtual void WriteFileFromTail(std::string_view content) const = 0;
        virtual std::string ReadFile() const = 0;
        virtual std::string ReadFile(size_t bytes) const = 0;
        virtual std::string ReadFile(size_t begin, size_t end) const = 0;
    };

    class IFileOperator
    {
    public:
        virtual ~IFileOperator() = default;

        virtual void RemoveFile() const = 0;
        virtual void RenameTo(std::string_view new_name) const = 0;
        virtual void Copy(const fs::path& dest_path) const = 0;
        virtual void MoveTo(const fs::path& dest_path) const = 0;
        virtual void SetPath(const fs::path& path) = 0;
    };

    class ITotalFileOperator : public IFileInfo, public IFileIO, public IFileOperator
    {
    public:
        virtual ~ITotalFileOperator() = default;
    };
    

    class FileObject : public ITotalFileOperator
    {
    public:
        FileObject(const Path& path)
        {
            _path = path;
        }

        mutable std::mutex _mutex;

    public:
        std::string MakeMetaData() const
        {
            std::string metaData;
            auto sys_time = std::chrono::clock_cast<std::chrono::system_clock>(std::chrono::system_clock::now());
            auto now_time = std::chrono::system_clock::to_time_t(sys_time);

            metaData += "File Name: " + GetFileName() + "\n";
            metaData += "Last Modify Time: \"";
            
            auto time_str = [&]()->std::string
            {
                std::string time_str;
                std::tm time {};

                #if defined(_WIN32)
                    localtime_s(&time, &now_time);
                #else
                    localtime_r(&now_time, &time);
                #endif

                time_str += std::to_string(time.tm_year + 1900) + "-";
                time_str += std::to_string(time.tm_mon + 1) + "-";
                time_str += std::to_string(time.tm_mday) + " ";
                time_str += std::to_string(time.tm_hour) + ":";
                time_str += std::to_string(time.tm_min) + ":";
                time_str += std::to_string(time.tm_sec);
                return time_str;
            }();
            
            metaData += time_str + "\"\n";
            metaData += "File Size: " + std::to_string(GetSize()) + " bytes\n";

            return metaData;
        }

    public:

        size_t GetSize() const noexcept override
        {
            return fs::file_size(_path);
        }

        std::string GetFileName() const noexcept override
        {
            return _path.filename().string();
        }

        std::string GetPathStr() const override
        {
            return _path.string();
        }

        std::tm GetFileCreateTime() const override
        {
            return GetFileModifyTime();
        }

        std::tm GetFileModifyTime() const override
        {
            std::tm time {};
            auto res = fs::last_write_time(_path);
            auto sys_time = std::chrono::clock_cast<std::chrono::system_clock>(res);
            auto t = std::chrono::system_clock::to_time_t(sys_time);

            #if defined(_WIN32)
                localtime_s(&time, &t);
            #else
                localtime_r(&t, &time);
            #endif
            
            return time;
        }

        void WriteFile(std::string_view content) const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, false);
            }

            

            {
                std::lock_guard<std::mutex> lock(_mutex);
                std::ofstream file(_path, std::ios::out);
                
                auto finalContent = MakeMetaData() + std::string(content);

                file.write(finalContent.data(), finalContent.size());
            }
        }

        void WriteFileMetaData(std::string_view content) const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, false);
            }

            {
                std::lock_guard<std::mutex> lock(_mutex);
                std::ofstream file(_path, std::ios::out);
                
                auto finalContent = MakeMetaData() + "\n" + std::string(content);

                file.write(finalContent.data(), finalContent.size());
            }
        }

        void WriteFileFromHead(std::string_view content) const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, false);
            }

            {
                std::lock_guard<std::mutex> lock(_mutex);
                std::ofstream file(_path, std::ios::app);
                
                file.seekp(0, std::ios::beg);

                auto finalContent = std::string(content) + "\n" + MakeMetaData();

                file.write(finalContent.data(), finalContent.size());
            }
        }
        
        void WriteFileFromTail(std::string_view content) const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, false);
            }

            {
                std::lock_guard<std::mutex> lock(_mutex);
                std::ofstream file(_path, std::ios::app);
                
                auto finalContent = MakeMetaData() + "\n" + std::string(content);

                file.write(finalContent.data(), finalContent.size());
            }
        }
        
        std::string ReadFile() const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, false);
            }
            std::string content;
            
            {
                std::lock_guard<std::mutex> lock(_mutex);
                std::ifstream inFile(_path, std::ios::in);
                std::stringstream ss;
                ss << inFile.rdbuf();
                content = ss.str();
            }

            return content;
        }

        std::string ReadFile(size_t bytes) const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, false);
            }

            std::string content;

            {
                std::lock_guard<std::mutex> lock(_mutex);
                std::ifstream inFile(_path, std::ios::in);
                std::stringstream ss;
                ss << inFile.rdbuf();
                content = ss.str();
            }

            return content.substr(0, bytes);
        }

        std::string ReadFile(size_t begin, size_t end) const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, false);
            }

            std::string content;

            {
                std::lock_guard<std::mutex> lock(_mutex);
                std::ifstream inFile(_path, std::ios::in);
                std::stringstream ss;
                ss << inFile.rdbuf();
                content = ss.str();
            }

            return content.substr(begin, end);
        }

        void RemoveFile() const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, false);
            }

            fs::remove(_path);
        }

        virtual void Copy(const fs::path& dest_path) const override
        {
            if(dest_path.empty())
            {
                FSLogCallback("[GwongDongFileSystem] path is empty, cannot set path.", Plugin_Logs::logLevel::warn, true);
                return;
            }
        }

        virtual void MoveTo(const fs::path& dest_path) const override
        {
            if(dest_path.empty())
            {
                FSLogCallback("[GwongDongFileSystem] path is empty, cannot set path.", Plugin_Logs::logLevel::warn, true);
                return;
            }
        }

        virtual void SetPath(const fs::path& path) override
        {
            if(path.empty())
            {
                FSLogCallback("[GwongDongFileSystem] path is empty, cannot set path.", Plugin_Logs::logLevel::warn, true);
                return;
            }
        }

        virtual void RenameTo(std::string_view new_name) const override
        {
            if(new_name.empty())
            {
                FSLogCallback("[GwongDongFileSystem] new_name is empty, cannot set path.", Plugin_Logs::logLevel::warn, true);
                return;
            }
        }

        bool Exists() const noexcept override
        {
            return fs::exists(_path);
        }

        bool IsRegularFile() const override
        {
            return fs::is_regular_file(_path);
        }

        bool IsDirectory() const override
        {
            return fs::is_directory(_path);
        }
    
    private:
        //保存path，但是不持有文件实例，文件实例应当由需要时RAII。
        fs::path _path;
    };


    //文件管理接口，我恨模板......好丑......
    class FileManager
    {
    public:
        static FileManager& GetInstance()
        {
            static FileManager instance;
            return instance;
        }
    private:
        
        FileManager() = default;
        ~FileManager() = default;

        mutable std::mutex mutex;
    public:
        //内容存储查询
        auto NGetFile(const fs::path& path) const
        {
            return std::find_if(baseFileArray.begin(), baseFileArray.end(), [&](const auto& TotalFileOperator){
                return TotalFileOperator->GetPathStr() == path.string();
            });
        }

        auto NGetFile(const std::string_view name)
        {
            return std::find_if(baseFileArray.begin(), baseFileArray.end(), [&](const auto& TotalFileOperator){
                return TotalFileOperator->GetFileName() == name;
            });
        }

    public:


        //TODO:NMakeFile重载的命名随机化。
        template<typename Ty, typename ...Args>
        Ty* NMakeFile(Args&&... args)
        {
            if constexpr (!std::is_base_of_v<ITotalFileOperator, Ty>)
            {
                FSLogCallback("[GwongDongFileSystem] T必须继承于ITotalFileOperator。", Plugin_Logs::logLevel::err, true);
                return nullptr;
            }

            auto ptr = std::make_shared<Ty>(std::forward<Args>(args)...);
            Ty* rawPtr = ptr.get();

            {
                std::lock_guard<std::mutex> lock(mutex);
                baseFileArray.emplace_back(std::move(ptr));
            }

            FSLogCallback(std::format("[GwongDongFileSystem] 文件创建成功。\n路径：{}", ptr->GetPathStr()), Plugin_Logs::logLevel::info, true);
            return rawPtr;
        }

        void NMakeFile(fs::path& path, std::string_view name, std::string_view fileNameExtension)
        {
            srand(time(nullptr));

            FSLogCallback("[GwongDongFileSystem] 正在创建文件。", Plugin_Logs::logLevel::info, true);

            std::string finalName;

            if(path.empty())
            {
                FSLogCallback("[GwongDongFileSystem] path is empty, will create file at this directory.", Plugin_Logs::logLevel::info, true);
                path = fs::current_path();
            }

            if(name.empty())
            {
                FSLogCallback("[GwongDongFileSystem] name is empty, using random name.", Plugin_Logs::logLevel::warn, true);
                
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    
                    finalName = []() {
                        auto now = std::chrono::system_clock::now();
                        auto now_ns = std::chrono::time_point_cast<std::chrono::nanoseconds>(now);
                        auto timestamp = now_ns.time_since_epoch().count();

                        thread_local std::mt19937_64 rng{std::random_device{}()};
                        std::uniform_int_distribution<uint64_t> dist;
                        uint64_t random_part = dist(rng);

                        std::ostringstream oss;
                        oss << std::hex << std::setfill('0')
                            << std::setw(16) << timestamp   // 16 位十六进制时间戳
                            << "-"
                            << std::setw(16) << random_part // 16 位十六进制随机数
                            << "-"
                            << std::setw(8) << std::this_thread::get_id();

                        return oss.str();
                    }();
                }

                FSLogCallback(std::format("[GwongDongFileSystem] Using name: {}", finalName), Plugin_Logs::logLevel::info, true);
            }
            else
            {
                finalName = name;
            }

            if(fileNameExtension.empty())
            {
                FSLogCallback("[GwongDongFileSystem] fileNameExtension is empty, using default.", Plugin_Logs::logLevel::warn, true);

                fileNameExtension = "nil";
            }

            path.append(finalName).replace_extension(fileNameExtension);

            {
                std::lock_guard<std::mutex> lock(mutex);
                auto ptr = std::make_shared<FileObject>(std::forward<fs::path>(path));
                baseFileArray.emplace_back(std::move(ptr));
                std::ofstream outFile(path);

                FSLogCallback(std::format("[GwongDongFileSystem] 文件创建成功。\n路径：{}", ptr->GetPathStr()), Plugin_Logs::logLevel::info, true);
            }
            
        }
        
        void IOBase()
        {
            
        }

        void NRenameFile(const fs::path& path, std::string_view newName) const
        {

        }
        void NDeleteFile(const fs::path& path) const
        {

        }
        void NCopyFile(const fs::path& from, const fs::path& to) const
        {

        }
        void NMoveFile(const fs::path& from, const fs::path& to) const
        {
            FSLogCallback(std::format("[GwongDongFileSystem] 尝试移动文件:{} -> {}。", from.string(), to.string()), Plugin_Logs::logLevel::info, true);
            
        }
        void NCreateDirectory(const fs::path& path) const
        {
            FSLogCallback("[GwongDongFileSystem] 创建目录。", Plugin_Logs::logLevel::info, true);
            fs::create_directory(path);
            FSLogCallback(std::format("[GwongDongFileSystem] 目录创建成功。\n路径：{}", path.string()), Plugin_Logs::logLevel::info, true);
        }
        void NDeleteDirectory(const fs::path& path, std::error_code& ec, size_t& rm_count) const
        {
            FSLogCallback("[GwongDongFileSystem] 删除目录。", Plugin_Logs::logLevel::info, true);
            fs::_Remove_all_dir(path, ec, rm_count);
        }

        void NGetFileArray(std::vector<ITotalFileOperator>& baseFileArray) const
        {

        }

    private:
        std::vector<std::shared_ptr<ITotalFileOperator>> baseFileArray;

    };


}

namespace GDfs = GwongDongFileSystem;







