#include <GwongDongFileSystem.hpp>


namespace GwongDongFileSystem
{
    LogCallBack FSLogCallback;
    void SetLogCallback(LogCallBack callback)
    {
        FSLogCallback = callback;
    }

    //基础文件实例类
    class FileBase : public ITotalFileOperator
    {
    public:
        FileBase()
        {

        }

        FileBase(const FileBase&) = delete;
        virtual ~FileBase() = default;

    public:

    void ReadFile(std::string& file) const override
    {

    }

    void DeleteFile() const override
    {

    }

    void RenameFile(std::string& file) const override
    {

    }

    public:
        std::string fileName;
        fs::path path;
        std::fstream file;
        std::tm createTime;
        std::tm modifyTime;
        std::tm accessTime;
        size_t fileSize;
        std::string Owner;
    };

    // class FileInstance : public FileBase
    // {
    // public:
    //     FileInstance()
    //     {

    //     }

    //     FileInstance(const FileInstance&) = delete;
    //     ~FileInstance() = default;

    // public:
    //     void GetSize(size_t& size) const override
    //     {

    //     }
    //     void GetFileName(std::string& name) const override
    //     {

    //     }
    //     void GetPathStr(std::string& path) const override
    //     {

    //     }
    //     void GetFileCreateTime(std::tm& time) const override
    //     {

    //     }
    //     void GetFileModifyTime(std::tm& time) const override
    //     {

    //     }
    //     void GetFileAccessTime(std::tm& time) const override
    //     {

    //     }
    //     void GetFile(std::string& file) const override
    //     {

    //     }
    //     void WriteFile(std::string& file) const override
    //     {

    //     }
    //     void CopyFile(std::string& file) const override
    //     {

    //     }
    //     void MoveFile(std::string& file) const override
    //     {

    //     }
    //     void PasteFile(std::string& file) const override
    //     {

    //     }
    //     void SetPath(std::string& path) const override
    //     {

    //     }

    // public:
        
    // };







    // const bool MakePath(const std::string_view string)
    // {
    //     return true;
    // }
    

    // const bool CheckPathExist(const Path& path) noexcept
    // {
    //     if(path.empty())
    //     {
    //         FSLogCallback("[GwongDongFileSystem] path is empty.", Plugin_Logs::logLevel::warn, false);
    //         return false;
    //     }

    //     if(fs::exists(path))
    //     {
    //         FSLogCallback("[GwongDongFileSystem] path is exist.", Plugin_Logs::logLevel::info, false);
    //         return true;
    //     }

    //     return false;
    // }



    // //创建文件
    // bool WriteFile(const Path& path)
    // {
    //     if(path.empty())
    //     {
    //         FSLogCallback("[GwongDongFileSystem] 输入路径为空，无法写入文件。", Plugin_Logs::logLevel::warn, false);
    //         return false;
    //     }

    //     if(!CheckPathExist(path))
    //     {
    //         FSLogCallback("[GwongDongFileSystem] 文件不存在，将创建文件。", Plugin_Logs::logLevel::warn, false);
    //         CreatFile(path);
    //     }

    //     std::ofstream file(path);

    //     return true;
    // }

}
