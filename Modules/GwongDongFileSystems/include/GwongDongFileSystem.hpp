#pragma once

#include <random>
#include <chrono>
#include <iomanip>
#include <fstream>
#include <functional>
#include <filesystem>
#include <loggings.hpp>

namespace fs = std::filesystem;
using Path = std::filesystem::path;
using FileTime = std::chrono::system_clock::time_point;


namespace GwongDongFileSystem
{
    //日志记录
    using LogCallBack = std::function<void(const std::string&, Plugin_Logs::logLevel, bool nowFlush)>;

    extern LogCallBack FSLogCallback;
    void SetLogCallback(LogCallBack callback);    

    //为文件类型预留接口，目前未使用。
    enum class FileType
    {
        RegularFile,
        Directory,
        SymbolicLink,
        Socket,
        CharacterDevice,
        BlockDevice,
        FIFO,
        Unknown,
        Other
    };

    enum class FileMode
    {
        Text,
        Binary
    };

    //写模式
    enum class WriteMode
    {
        Overwrite, Append, Prepend
    };





    //基础文件接口(包含MetaData接口和文件操作接口)
    //拆解文件接口
    class IFileInfo
    {
    public:
        virtual ~IFileInfo() = default;

        virtual size_t GetSize() const = 0;
        virtual std::string GetFileName() const = 0;
        virtual std::string GetPathStr() const = 0;
        virtual FileTime GetFileCreateTime() const = 0;
        virtual FileTime GetFileModifyTime() const = 0;
        virtual bool Exists() const noexcept = 0;
        virtual bool IsDirectory() const = 0;
        virtual bool IsRegularFile() const = 0;
    };

    class IFileIO
    {
    public:
        virtual ~IFileIO() = default;

        virtual void WriteFile(std::string_view content, WriteMode mode) const = 0;
        virtual std::string ReadFile() const = 0;
        virtual std::string ReadFile(size_t bytes) const = 0;
        virtual std::string ReadFile(size_t offset, size_t length) const = 0;
        virtual std::string ExtractMetaData() const = 0;
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
    
    


    //MetaData 被存储在文件尾。
    struct FileMetaData
    {
    public:
        ~FileMetaData() = default;

        FileMetaData()
        {
            _name = "NULL";
            _createTime = FileTime{};
            _modifyTime = FileTime{};
            _owner = "NULL";
            _informationLeave = "NULL";
            _extension = "nil";
            _fileType = FileType::RegularFile;
        }

        virtual std::string UpdateMetaData()
        {
            //更新时间
            _modifyTime = std::chrono::system_clock::now();

            std::string metaData;
            std::string fileTypeStr = [&]() {
                switch (_fileType)
                {
                case FileType::RegularFile:
                    return "RegularFile";
                case FileType::Directory:
                    return "Directory";
                case FileType::SymbolicLink:
                    return "SymbolicLink";
                case FileType::Socket:
                    return "Socket";
                case FileType::CharacterDevice:
                    return "CharacterDevice";
                case FileType::BlockDevice:
                    return "BlockDevice";
                case FileType::FIFO:
                    return "FIFO";
                case FileType::Unknown:
                    return "Unknown";
                case FileType::Other:
                    return "Other";
                }
            }();


            metaData += "&_Begin_& Begin MetaData:\n";
            metaData += "CreateTime: " + std::format("{:%Y-%m-%d %H:%M:%S}", _createTime) + "\n";
            metaData += "ModifyTime: " + std::format("{:%Y-%m-%d %H:%M:%S}", _modifyTime) + "\n";
            metaData += "FileType: " + fileTypeStr + "\n";
            metaData += "Name: " + _name + "\n";
            metaData += "Owner: " + _owner + "\n";
            metaData += "Info: " + _informationLeave + "\n";
            metaData += "Extension: " + _extension + "\n";
            metaData += std::string("&_End_& End MetaData\n");

            if(sizeof(metaData) > 4096)
            {
                FSLogCallback(std::format("[GwongDongFileSystem] MetaData大小超过了4096字节，当前大小为{}字节。请精简MetaData内容！", sizeof(metaData)), Plugin_Logs::logLevel::err, true);
            }

            return metaData;
        }

        virtual void SetFileType(const FileType& type)
        {
            _fileType = type;
        }

        virtual void SetExtension(const fs::path& path)
        {
            _extension = path.extension().string();
        }

        virtual void SetOwner(std::string_view owner)
        {
            _owner = std::move(owner);
        }

        virtual void SetCreateTime(const FileTime& time)
        {
            _createTime = time;
        }

        virtual void SetModifyTime()
        {
            _modifyTime = std::chrono::system_clock::now();
        }

        virtual void SetName(std::string_view name)
        {
            _name = std::move(name);
        }

        virtual void SetInformationLeave(std::string_view informationLeave)
        {
            _informationLeave = std::move(informationLeave);
        }

        virtual std::string GetName() const
        {
            return _name;
        }

        virtual std::string GetOwner() const
        {
            return _owner;
        }

        virtual std::string GetExtension() const
        {
            return _extension;
        }

        virtual FileTime GetCreateTime() const
        {
            return _createTime;
        }

        virtual FileTime GetModifyTime() const
        {
            return _modifyTime;
        }

        virtual FileType GetFileType() const
        {
            return _fileType;
        }

        virtual std::string GetInformationLeave() const
        {
            return _informationLeave;
        }

    private: 
        std::string _name;
        FileTime _createTime;
        FileTime _modifyTime;
        std::string _owner;
        std::string _informationLeave;
        std::string _extension;
        FileType _fileType;
    };
    
    class MetaDataSerializer
    {
    public:
        virtual std::vector<uint8_t> SerializeMetaDataBinary(const FileMetaData& metaData) const
        {

        };

        virtual FileMetaData DeserializeMetaDataBinary(const std::vector<uint8_t>& data) const
        {

        }

        virtual FileMetaData DeserializeMetaDataText(const std::string& content) const
        {
            return [&](std::string_view content) 
                -> FileMetaData
            {
                FileMetaData metaData;

                auto beginPos = content.find("&_Begin_&");
                auto endPos = content.find("&_End_&");

                if(beginPos == std::string::npos || endPos == std::string::npos || endPos <= beginPos)
                {
                    FSLogCallback("[GwongDongFileSystem] 文件不存在MetaData，文件可能已损坏或你误打开了非该文件系统所管理的文件。", Plugin_Logs::logLevel::warn, true);
                    return metaData;
                }
                
                //TODO: 解析MetaData


                return metaData;
            }(content);
        }
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

        //INFO

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

        FileTime GetFileCreateTime() const override
        {
            // #ifdef _GetCreationTime_From_MetaData
                
                //auto MetaData = 
            



            // #endif
        }

        FileTime GetFileModifyTime() const override
        {
            auto last_write = fs::last_write_time(_path);
            auto res = std::chrono::clock_cast<std::chrono::system_clock>(last_write);
            
            return res;
        }

        //IO

        void WriteFile(std::string_view content, WriteMode mode) const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, false);
            }

            

            {
                std::lock_guard<std::mutex> lock(_mutex);
                std::ofstream file(_path, std::ios::out);
                
                auto finalContent = std::string(content);

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

        std::string ReadFile(size_t offset, size_t length) const override
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

            return content.substr(offset, length);
        }

        std::string ExtractMetaData() const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, false);
            }

            const std::string metaDataStart = "&_Begin_& Begin MetaData";
            const std::string metaDataEnd = "&_End_& End MetaData";


            {
                std::lock_guard<std::mutex> lock(_mutex);

                std::ifstream inFile(_path, std::ios::in);
                auto startPos = std::string::npos;
                auto endPos = std::string::npos;
                

                //只读取前8kb，如果没有找到MetaData则代表该文件不存在MetaData或文件以损坏。
                const size_t MAX_CHUNK_SIZE = 8192;
                std::string buffer(MAX_CHUNK_SIZE, '\0');

                //读取文件内容到buffer，并寻找MetaData的起始和结束位置。
                inFile.read(buffer.data(), MAX_CHUNK_SIZE);
                startPos = buffer.find(metaDataStart);
                endPos = buffer.find(metaDataEnd);

                //如果没有找到MetaData的起始或结束标志，或者结束标志在起始标志之前，则认为文件不存在MetaData。
                if(startPos == std::string::npos || endPos == std::string::npos || endPos <= startPos)
                {
                    FSLogCallback("[GwongDongFileSystem] 文件不存在MetaData，文件可能已损坏或你误打开了非该文件系统所管理的文件。", Plugin_Logs::logLevel::warn, true);
                    return "";
                }

                std::string metaData = buffer.substr(startPos + metaDataStart.size(), endPos - startPos - metaDataStart.size());

                return metaData;
            }
            
            throw std::runtime_error("[GwongDongFileSystem] 你是怎么到达这里的？！你对ExtractMetaData函数做了什么？！");
        }

        //OPERATOR

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
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::warn, true);
                return;
            }

            fs::copy(_path, dest_path);
        }

        virtual void MoveTo(const fs::path& dest_path) const override
        {
            if(dest_path.empty())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::warn, true);
                return;
            }
            fs::copy(_path, dest_path);
            fs::remove(_path);
        }

        virtual void SetPath(const fs::path& path) override
        {
            if(path.empty())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::warn, true);
                return;
            }
        }

        virtual void RenameTo(std::string_view new_name) const override
        {
            if(new_name.empty())
            {
                FSLogCallback("[GwongDongFileSystem] 新文件名为空。", Plugin_Logs::logLevel::warn, true);
                return;
            }

            fs::rename(_path, _path.parent_path() / new_name);
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

        void NRenameFile(const fs::path& path, std::string_view newName) 
        {
            auto file = NGetFile(path);

            if(file == baseFileArray.end())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::warn, true);
                return;
            }

            file->get()->RenameTo(newName);
        }
        
        void NDeleteFile(const fs::path& path)
        {
            auto file = NGetFile(path);

            if(file == baseFileArray.end())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::warn, true);
                return;
            }

            file->get()->RemoveFile();

            FSLogCallback(std::format("[GwongDongFileSystem] 文件删除成功。\n路径：{}", path.string()), Plugin_Logs::logLevel::info, true);
        }
        
        void NCopyFile(const fs::path& from, const fs::path& to) const
        {
            auto file = NGetFile(from);

            if(file == baseFileArray.end())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::warn, true);
                return;
            }

            file->get()->Copy(to);

            FSLogCallback(std::format("[GwongDongFileSystem] 复制文件:{} -> {}。", from.string(), to.string()), Plugin_Logs::logLevel::info, true);
        }
        
        void NMoveFile(const fs::path& from, const fs::path& to) const
        {
            auto file = NGetFile(from);

            if(file == baseFileArray.end())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::warn, true);
                return;
            }

            file->get()->MoveTo(to);

            FSLogCallback(std::format("[GwongDongFileSystem] 移动文件:{} -> {}。", from.string(), to.string()), Plugin_Logs::logLevel::info, true);
        }
        
        void NCreateDirectory(const fs::path& path) const
        {
            FSLogCallback("[GwongDongFileSystem] 创建目录。", Plugin_Logs::logLevel::info, true);
            fs::create_directory(path);
            FSLogCallback(std::format("[GwongDongFileSystem] 目录创建成功。\n路径：{}", path.string()), Plugin_Logs::logLevel::info, true);
        }
        
        void NDeleteDirectory(const fs::path& path, std::error_code& ec, size_t& rm_count) const
        {
            FSLogCallback(std::format("[GwongDongFileSystem] 删除目录: \n {} \n及其目录中文件。", path.string()), Plugin_Logs::logLevel::info, true);
            fs::remove_all(path);
        }

        auto& NGetFileArray() const
        {
            return baseFileArray;
        }

    private:
        std::vector<std::shared_ptr<ITotalFileOperator>> baseFileArray;

    };


}

namespace GDfs = GwongDongFileSystem;