#pragma once

#include <chrono>
#include <fstream>
#include <functional>
#include <filesystem>
#include <loggings.hpp>

namespace fs = std::filesystem;
using Path = std::filesystem::path;


namespace GwongDongFileSystem
{
    //日志记录
    using LogCallBack = std::function<void(const std::string&, Plugin_Logs::logLevel, bool nowFlush)>;
    void SetLogCallback(LogCallBack callback);

    class IBaseFile
    {
    public:
        IBaseFile() = default;
        virtual ~IBaseFile() = default;

    public:
        virtual void GetSize(size_t& size) const = 0;
        virtual void GetFileName(std::string_view name) const = 0;
        virtual void GetPath(std::string_view path) const = 0;
        virtual void GetFileCreateTime(std::tm& time) const = 0;
        virtual void GetFileModifyTime(std::tm& time) const = 0;
        virtual void GetFileAccessTime(std::tm& time) const = 0;
        virtual void GetFile(std::string& file) const = 0;
        virtual void WriteFile(std::string& file) const = 0;
        virtual void ReadFile(std::string& file) const = 0;
        virtual void DeleteFile() const = 0;
        virtual void RenameFile(std::string& file) const = 0;
        virtual void CopyFile(std::string& file) const = 0;
        virtual void MoveFile(std::string& file) const = 0;
        virtual void CreatFile(std::string& file) const = 0;
        virtual void PasteFile(std::string& file) const = 0;
        virtual void SetPath(std::string& path) const = 0;
    };




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


}

namespace GDfs = GwongDongFileSystem;







