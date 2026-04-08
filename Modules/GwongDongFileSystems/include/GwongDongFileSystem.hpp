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

    //IFileManager - > IFileOperator -> Do Work;
    //IFileManager 作为接口，提供管理和获取IBaseFile的功能，如创建、删除、重命名文件等。
    //IFileOperator 作为中间接口，提供文件的基本属性和操作，如获取文件大小、文件名、路径等。

    //基础文件接口(包含MetaData接口和文件操作接口)

    
    class ITotalFileOperator
    {
    public:
        ITotalFileOperator() = default;
        virtual ~ITotalFileOperator() = default;
        
    public:

        virtual size_t GetSize() const = 0;
        virtual std::string GetFileName() const = 0;
        virtual std::string GetPathStr() const = 0;
        virtual std::tm GetFileCreateTime() const = 0;
        virtual std::tm GetFileModifyTime() const = 0;
        virtual void WriteFile(std::string& file) const = 0;
        virtual void ReadFile(std::string& file) const = 0;
        virtual void DeleteFile() const = 0;
        virtual void RenameFile(std::string& file) const = 0;
        virtual void CopyFile(std::string& file) const = 0;
        virtual void MoveFile(std::string& file) const = 0;
        virtual bool Exists() const noexcept= 0;
        virtual bool IsDirectory() const = 0;
        virtual bool IsRegularFile() const = 0;

    };


    //拆解文件接口
/*
// class IFileInfo
// {
// public:
//     virtual ~IFileInfo() = default;

//     virtual size_t GetSize() const = 0;
//     virtual std::string GetFileName() const = 0;
//     virtual std::string GetPathStr() const = 0;
//     virtual std::tm GetFileCreateTime() const = 0;
//     virtual std::tm GetFileModifyTime() const = 0;
//     virtual bool Exists() const noexcept = 0;
//     virtual bool IsDirectory() const = 0;
//     virtual bool IsRegularFile() const = 0;
// };

// class IFileIO
// {
// private:
//     mutable std::mutex _mutex;

// public:
//     virtual ~IFileIO() = default;

//     virtual void WriteFile(const std::string& content) const = 0;
//     virtual void WriteFile(const std::string&& content) const = 0;
//     virtual void WriteFileMetaData(const std::string& content) const = 0;
//     virtual void WriteFileMetaData(const std::string&& content) const = 0;
//     virtual void WriteFileFromHead(const std::string& content) const = 0;
//     virtual void WriteFileFromHead(const std::string&& content) const = 0;
//     virtual void WriteFileFromTail(const std::string& content) const = 0;
//     virtual void WriteFileFromTail(const std::string&& content) const = 0;
//     virtual void WriteFileTotal(const std::string& content) const = 0;
//     virtual void WriteFileTotal(const std::string&& content) const = 0;
//     virtual std::string ReadFile() const = 0;
// };

// class IFileOperator
// {
// public:
//     virtual ~IFileOperator() = default;

//     virtual void DeleteFile() const = 0;
//     virtual void RenameFile(const std::string& new_name) const = 0;
//     virtual void CopyFile(const std::string& dest_path) const = 0;
//     virtual void MoveFile(const std::string& dest_path) const = 0;
//     virtual void SetPath(const std::string& path) = 0;
// };
*/

    

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

        void WriteFile(std::string& content) const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, false);
            }

            {
                std::lock_guard<std::mutex> lock(_mutex);
                std::ofstream file(_path, std::ios::out);
                
                auto metaData = MakeMetaData();
                content = metaData + content;
                file.write(content.data(), content.size());
            }
        }

        void ReadFile(std::string& content) const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, false);
            }
            
            {
                std::lock_guard<std::mutex> lock(_mutex);
                std::ifstream inFile(_path, std::ios::in);
                std::stringstream ss;
                ss << inFile.rdbuf();
                content = ss.str();
            }
        }

        void DeleteFile() const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, false);
            }

            fs::remove(_path);
        }

        void RenameFile(std::string& new_name) const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, false);
            }

            fs::rename(_path, new_name);
        }

        void CopyFile(std::string& dest_path) const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, false);
            }

            fs::copy(_path, dest_path);
        }

        void MoveFile(std::string& dest_path) const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, false);
            }

            fs::rename(_path, dest_path);
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

    /*
    class BaseFile : public IBaseFile
    { 
    public:
        BaseFile() = default;
        ~BaseFile() = default;

    public:
        virtual void GetSize(size_t& size) const override
        {
            size = fs::file_size(path);
        }
        virtual void GetFileName(std::string& name) const override
        {
            name = path.filename().string();
        }
        virtual void GetPathStr(std::string& path) const override
        {
            path = path.c_str();
        }
        virtual void GetFileModifyTime(std::tm& time) const override
        {
            auto res = fs::last_write_time(path);
            auto sys_time = std::chrono::file_clock::to_utc(res);
            
        }
        virtual void GetFileAccessTime(std::tm& time) const override
        {
            GetFileModifyTime(time);
        }
        virtual void GetFile(std::string& file) const override
        {
            file = file.c_str();
        }
        virtual void WriteFile(std::string& file) const override
        {

        }
        virtual void ReadFile(std::string& file) const override
        {

        }
        virtual void DeleteFile() const override
        {

        }
        virtual void RenameFile(std::string& file) const override
        {
            
        }
    private:
        std::fstream& file;
        fs::path& path;
    };

    */

    

    //文件管理接口
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

        void NMakeFile(fs::path& path, std::string_view name, std::string_view fileNameExtension) const
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

            FSLogCallback("[GwongDongFileSystem] Create file: " + path.string(), Plugin_Logs::logLevel::info, true);

            {
                std::lock_guard<std::mutex> lock(mutex);
                
                FileObject fileObject(path);
                std::ofstream outFile(path);
            }
            
            
        }
        void NWriteFile(const fs::path& path) const
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

        }
        void NCreateDirectory(const fs::path& path) const
        {

        }
        void NDeleteDirectory(const fs::path& path) const
        {

        }
        void NGetBaseFile(ITotalFileOperator& baseFile) const
        {

        }
        void NGetFile(ITotalFileOperator& BaseFile) const
        {

        }
        void NGetFileArray(std::vector<ITotalFileOperator>& baseFileArray) const
        {

        }

    private:
        std::vector<FileObject> baseFileArray;

    };



    /*
    
    //创建路径
    const bool MakePath(const char* string);
    const bool MakePath(const std::string& string);
    const bool MakePath(const std::string_view string);
    const bool MakePathFormCurrent(const Path& path) noexcept;

    //设置当前路径
    bool SetCurrentPath(const Path& path);

    //其他操作
    const bool CheckPathExist(const Path& path) noexcept;
    const bool ReadPresets(const Path& path);
    const bool GetSize(const Path& path);
    
    //文件或目录操作
    bool Copy(const Path& from, const Path& to);
    bool Remove(const Path& path);
    bool Rename(const Path& from, const Path& to);
    const bool Space(const Path& path);




    //Fstream
    bool CreatFile(const Path& path);
    bool WriteFile(const Path& path);
    const bool ReadFile(const Path& path);
    
    
    */
    


}

namespace GDfs = GwongDongFileSystem;







