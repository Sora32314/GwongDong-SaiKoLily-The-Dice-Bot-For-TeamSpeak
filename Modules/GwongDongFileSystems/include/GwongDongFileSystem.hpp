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

        virtual void WriteFile(std::string_view content, WriteMode mode) = 0;
        virtual std::string ReadFile() const = 0;
        virtual std::string ReadFile(size_t bytes) const = 0;
        virtual std::string ReadFile(size_t offset, size_t length) const = 0;

        virtual std::string ReadKiloBytes(size_t kiloBytes) const = 0;
        virtual std::string ReadKiloBytes(size_t kiloBytes, size_t offsetKiloBytes) const = 0;

        virtual std::string ReadBytes(size_t bytes) const = 0;
        virtual std::string ReadBytes(size_t bytes, size_t offsetBytes) const = 0;
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
    
    


    //MetaData 被存储在文件头。
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
            std::string fileTypeStr;
            switch (_fileType)
            {
            case FileType::RegularFile:
                fileTypeStr = "RegularFile";
                break;
            case FileType::Directory:
                fileTypeStr = "Directory";
                break;
            case FileType::SymbolicLink:
                fileTypeStr = "SymbolicLink";
                break;
            case FileType::Socket:
                fileTypeStr = "Socket";
                break;
            case FileType::CharacterDevice:
                fileTypeStr = "CharacterDevice";
                break;
            case FileType::BlockDevice:
                fileTypeStr = "BlockDevice";
                break;
            case FileType::FIFO:
                fileTypeStr = "FIFO";
                break;
            case FileType::Unknown:
                fileTypeStr = "Unknown";
                break;
            case FileType::Other:
                fileTypeStr = "Other";
                break;
            default:
                throw new std::runtime_error("[GwongDongFileSystem] FileType未定义！");
            }


            metaData += "&_Begin_& Begin MetaData:\n";
            metaData += "CreateTime: " + std::format("{:%Y-%m-%d %H:%M:%S}", _createTime) + "\n";
            metaData += "ModifyTime: " + std::format("{:%Y-%m-%d %H:%M:%S}", _modifyTime) + "\n";
            metaData += "FileType: " + fileTypeStr + "\n";
            metaData += "Name: " + _name + "\n";
            metaData += "Owner: " + _owner + "\n";
            metaData += "Info: " + _informationLeave + "\n";
            metaData += "Extension: " + _extension + "\n";
            metaData += std::string("&_End_& End MetaData\n");

            if(metaData.size() > 4096)
            {
                FSLogCallback(std::format("[GwongDongFileSystem] MetaData大小超过了4096字节，当前大小为{}字节。请精简MetaData内容！", metaData.size()), Plugin_Logs::logLevel::err, true);
            }

            return metaData;
        }

        virtual void SetFileType(const FileType& type)
        {
            _fileType = type;
        }

        virtual void SetExtension(const std::string& extension)
        {
            _extension = std::move(extension);
        }

        virtual void SetExtension(const fs::path& path)
        {
            _extension = path.extension().string();
        }

        virtual void SetOwner(std::string_view owner)
        {
            _owner = std::move(owner);
        }

        virtual void InitCreateTime()
        {
            _createTime = std::chrono::system_clock::now();
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
        static bool HasMetaData(const std::string& content)
        {
            return [&]() -> bool
            {
                auto beginPos = content.find("&_Begin_& Begin MetaData:");
                auto endPos = content.find("&_End_& End MetaData");

                if(beginPos == std::string::npos || endPos == std::string::npos || endPos <= beginPos)
                {
                    #ifdef _DEBUG
                        FSLogCallback("[GwongDongFileSystem] HASMETADATA：文件不存在MetaData，文件可能已损坏或你误打开了非该文件系统所管理的文件。", Plugin_Logs::logLevel::warn, true);
                    #endif
                    return false;
                }
                
                return true;
            }();
        };

        static std::vector<uint8_t> SerializeMetaDataBinary(const FileMetaData& metaData)
        {

        };

        static FileMetaData DeserializeMetaDataBinary(const std::vector<uint8_t>& data)
        {

        }

        static FileMetaData DeserializeMetaDataText(const std::string& content)
        {
            return [&](std::string_view content) 
                -> FileMetaData
            {
                FileMetaData metaData;

                auto beginPos = content.find("&_Begin_& Begin MetaData:");
                auto endPos = content.find("&_End_& End MetaData");

                if(beginPos == std::string::npos || endPos == std::string::npos || endPos <= beginPos)
                {
                    FSLogCallback("[GwongDongFileSystem] DESERIALIZE_METADATA_TEXT：文件不存在MetaData，文件可能已损坏或你误打开了非该文件系统所管理的文件。", Plugin_Logs::logLevel::warn, true);
                    return metaData;
                }
                
                //TODO: 解析MetaData
                beginPos += std::string("&_Begin_& Begin MetaData:").size();
                endPos -= 1;

                auto cut = content.substr(beginPos, endPos - beginPos);

                [&]()
                {
                    if(cut.find("CreateTime: ") != std::string::npos)
                    {
                        std::string timeStr = cut.substr(cut.find("CreateTime: ") + std::string("CreateTime: ").size()).data();
                        timeStr = timeStr.substr(0, timeStr.find("\n")).data();

                        #ifdef _DEBUG
                            FSLogCallback(std::format("[GwongDongFileSystem] timeStr:{}", timeStr), Plugin_Logs::logLevel::warn, true);
                        #endif

                        std::istringstream ss(timeStr);
                        std::chrono::system_clock::time_point parseCreateTime;
                        ss >> std::chrono::parse("%Y-%m-%d %H:%M:%S", parseCreateTime);
                        
                        if(!ss.fail())
                        {
                            metaData.SetCreateTime(parseCreateTime);
                        }
                        else
                        {
                            FSLogCallback("[GwongDongFileSystem] DESERIALIZE_METADATA_TEXT：文件存在MetaData::create_time，但无法解析时间，请检查文件。", Plugin_Logs::logLevel::warn, true);
                        }
                    }

                    if(cut.find("ModifyTime: ") != std::string::npos)
                    {
                        metaData.SetModifyTime();
                    }

                    if(cut.find("Name: ") != std::string::npos)
                    {
                        std::string name = cut.substr(cut.find("Name: ") + std::string("Name: ").size()).data();
                        name = name.substr(0, name.find("\n")).data();
                        metaData.SetName(name);

                        #ifdef _DEBUG
                            FSLogCallback(std::format("[GwongDongFileSystem] MetaData::Name:{}", metaData.GetName()), Plugin_Logs::logLevel::warn, true);
                        #endif
                    }

                    if(cut.find("Owner: ") != std::string::npos)
                    {
                        std::string owner = cut.substr(cut.find("Owner: ") + std::string("Owner: ").size()).data();
                        owner = owner.substr(0, owner.find("\n")).data();
                        metaData.SetOwner(owner);

                        #ifdef _DEBUG
                            FSLogCallback(std::format("[GwongDongFileSystem] MetaData::Owner:{}", metaData.GetOwner()), Plugin_Logs::logLevel::warn, true);
                        #endif
                    }
                    
                    if(cut.find("Extension: ") != std::string::npos)
                    {
                        std::string extension = cut.substr(cut.find("Extension: ") + std::string("Extension: ").size()).data();
                        extension = extension.substr(0, extension.find("\n")).data();
                        metaData.SetExtension(extension);

                        #ifdef _DEBUG
                            FSLogCallback(std::format("[GwongDongFileSystem] MetaData::Extension:{}", metaData.GetExtension()), Plugin_Logs::logLevel::warn, true);
                        #endif
                    }
                    
                    if(cut.find("FileType: ") != std::string::npos)
                    {
                        auto type = cut.substr(cut.find("FileType: ") + std::string("FileType: ").size());

                        switch(type[0])
                        {
                        case 'D':
                            metaData.SetFileType(FileType::Directory);
                            break;
                        case 'R':
                            metaData.SetFileType(FileType::RegularFile);
                            break;
                        case 'C':
                            metaData.SetFileType(FileType::CharacterDevice);
                        break;
                        case 'B':
                            metaData.SetFileType(FileType::BlockDevice);
                        break;
                        case 'S':
                            if(type[1] == 'O')
                                metaData.SetFileType(FileType::Socket);
                            else
                                metaData.SetFileType(FileType::SymbolicLink);
                        break;
                        case 'F':
                            metaData.SetFileType(FileType::FIFO);
                        break;
                        case 'O':
                            metaData.SetFileType(FileType::Other);
                        break;
                        case 'U':
                            metaData.SetFileType(FileType::Unknown);
                        break;
                        default:
                            FSLogCallback("[GwongDongFileSystem] DESERIALIZE_METADATA_TEXT：文件存在MetaData::file_type，但无法解析文件类型，请检查文件。", Plugin_Logs::logLevel::warn, true);
                            break;
                        }
                    
                    }
                    
                    if(cut.find("Info: ") != std::string::npos)
                    {
                        std::string info = cut.substr(cut.find("Info: ") + std::string("Info: ").size()).data();
                        info = info.substr(0, info.find("\n")).data();
                        metaData.SetInformationLeave(info);
                        
                        #ifdef _DEBUG
                            FSLogCallback(std::format("[GwongDongFileSystem] MetaData::InformationLeave:{}", metaData.GetInformationLeave()), Plugin_Logs::logLevel::warn, true);
                        #endif
                    }

                }();

                return metaData;
            }(content);
        }
    };



    class FileObject : public ITotalFileOperator
    {
    public:
        FileObject(const Path& path, FileMetaData& metaData)
            : _path(path), _metaData(std::move(metaData))
        {
            _metaData.InitCreateTime();
            std::string res = _metaData.UpdateMetaData();

            #ifdef _DEBUG
            FSLogCallback(std::format("[GwongDongFileSystem] MetaData:{}", res), Plugin_Logs::logLevel::info, true);
            #endif

            WriteFile(res, WriteMode::Overwrite);
        }

        //此构造函数仅用于从已存在的文件创建FileObject实例，MetaData由ExtractMetaData函数解析获得。
        FileObject(const Path& path)
            : _path(path)
        {
            //从路径中解析已存在的MetaData
            _metaData = MetaDataSerializer::DeserializeMetaDataText(ExtractMetaData());
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
            auto MetaDataString = ExtractMetaData();
            FileMetaData metaData = MetaDataSerializer::DeserializeMetaDataText(MetaDataString);


            return metaData.GetCreateTime();
        }

        FileTime GetFileModifyTime() const override
        {
            auto MetaDataString = ExtractMetaData();
            FileMetaData metaData = MetaDataSerializer::DeserializeMetaDataText(MetaDataString);
            
            return metaData.GetModifyTime();
        }


        //TODO: 优化IO分块或直接使用mmap进行内存映射。
        void WriteFile(std::string_view content, WriteMode mode) override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, true);
                return;
            }

            switch(mode)
            {
            case WriteMode::Overwrite:
            {
                _metaData.SetModifyTime();
                
                auto res = ReadKiloBytes(8);
                std::string finalContent;

                if(MetaDataSerializer::HasMetaData(res))
                {
                    auto metaData = MetaDataSerializer::DeserializeMetaDataText(res);

                    #ifdef _DEBUG
                        FSLogCallback(std::format("[GwongDongFileSystem] Write::Override:MetaData:{}", res), Plugin_Logs::logLevel::info, true);
                    #endif

                    _metaData = std::move(metaData);

                    finalContent = _metaData.UpdateMetaData() + std::string(content);
                }
                else
                {
                    finalContent = _metaData.UpdateMetaData() + std::string(content);
                }
                
                std::ofstream file(_path, std::ios::out | std::ios::trunc);

                file.write(finalContent.data(), finalContent.size());

                file.flush();
                break;
            }  
            case WriteMode::Append:
            {
                //8kb分块读取
                auto chunk_bytes = ReadKiloBytes(8);
                const std::string metaDataEnd = "&_End_& End MetaData";
                auto end_pos = chunk_bytes.find(metaDataEnd) + metaDataEnd.size();

                if(MetaDataSerializer::HasMetaData(chunk_bytes))
                {
                    auto cachedMetaData = MetaDataSerializer::DeserializeMetaDataText(chunk_bytes);

                    std::string chunk_without_metadata = chunk_bytes.substr(end_pos, chunk_bytes.size());

                    _metaData = std::move(cachedMetaData);

                    chunk_bytes = _metaData.UpdateMetaData() + chunk_without_metadata;

                    {
                        std::lock_guard<std::mutex> lock(_mutex);
                        std::fstream file(_path, std::ios::in | std::ios::out);
                        file.seekp(0, std::ios::beg);
                        file.write(chunk_bytes.data(), chunk_bytes.size());

                        file.flush();
                    }
                }
                else
                {
                    auto cachedMetaData = _metaData.UpdateMetaData();

                    {
                        std::lock_guard<std::mutex> lock(_mutex);
                        std::fstream file(_path, std::ios::in | std::ios::out);
                        file.seekp(0, std::ios::beg);
                        file.write(cachedMetaData.data(), cachedMetaData.size());

                        file.flush();
                    }

                }
                
                std::lock_guard<std::mutex> lock(_mutex);

                std::ofstream file(_path, std::ios::out | std::ios::app);

                file.write(content.data(), content.size());

                file.flush();
                break;
            }
            case WriteMode::Prepend:
            {
                break;
            }
            default:
                FSLogCallback("[GwongDongFileSystem] 不支持的写入模式。", Plugin_Logs::logLevel::warn, true);
                break;
            }
        }
        
        std::string ReadFile() const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, true);
                return "";
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
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, true);
                return "";
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
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, true);
                return "";
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

        std::string ReadKiloBytes(size_t bytes) const override
        {
            std::lock_guard<std::mutex> lock(_mutex);
            std::ifstream inFile(_path, std::ios::in);

            const size_t MAX_CHUNK_SIZE = bytes * 1024;
            std::string buffer(MAX_CHUNK_SIZE, '\0');

            inFile.read(buffer.data(), MAX_CHUNK_SIZE);

            std::streamsize bytesRead = inFile.gcount(); 

            buffer.resize(bytesRead);

            #ifdef _DEBUG
            FSLogCallback(std::format("[GwongDongFileSystem] ReadKiloBytes - 实际读取字节:{} 内容:{}", bytesRead, buffer), Plugin_Logs::logLevel::info, true);
            #endif

            return buffer;
        }

        std::string ReadKiloBytes(size_t bytes, size_t offsetBytes) const override
        {
            std::lock_guard<std::mutex> lock(_mutex);
            std::ifstream inFile(_path, std::ios::in);

            inFile.seekg(offsetBytes, std::ios::beg);

            const size_t MAX_CHUNK_SIZE = bytes * 1024;

            std::string buffer(MAX_CHUNK_SIZE, '\0');
            inFile.read(buffer.data(), MAX_CHUNK_SIZE);

            std::streamsize bytesRead = inFile.gcount(); 

            buffer.resize(bytesRead);

            #ifdef _DEBUG
            FSLogCallback(std::format("[GwongDongFileSystem] ReadKiloBytes - 实际读取字节:{} 内容:{}", bytesRead, buffer), Plugin_Logs::logLevel::info, true);
            #endif

            return buffer;
        }

        std::string ReadBytes(size_t bytes) const override
        {
            std::lock_guard<std::mutex> lock(_mutex);
            std::ifstream inFile(_path, std::ios::in);

            std::string buffer(bytes, '\0');
            inFile.read(buffer.data(), bytes);

            std::streamsize bytesRead = inFile.gcount();

            buffer.resize(bytesRead);
            #ifdef _DEBUG
            FSLogCallback(std::format("[GwongDongFileSystem] ReadBytes - 读取字节:{} 内容:{}", bytesRead, buffer), Plugin_Logs::logLevel::info, true);
            #endif

            return buffer;
        }

        std::string ReadBytes(size_t bytes, size_t offsetBytes) const override
        {
            std::lock_guard<std::mutex> lock(_mutex);
            std::ifstream inFile(_path, std::ios::in);

            inFile.seekg(offsetBytes, std::ios::beg);

            std::string buffer(bytes, '\0');
            inFile.read(buffer.data(), bytes);

            std::streamsize bytesRead = inFile.gcount();

            buffer.resize(bytesRead);
            #ifdef _DEBUG
            FSLogCallback(std::format("[GwongDongFileSystem] ReadBytes - 读取字节:{} 内容:{}", bytesRead, buffer), Plugin_Logs::logLevel::info, true);
            #endif

            return buffer;
        }

        std::string ExtractMetaData() const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 无法解析MetaData，文件不存在。", Plugin_Logs::logLevel::info, true);
                return "";
            }

            const std::string metaDataStart = "&_Begin_& Begin MetaData";
            const std::string metaDataEnd = "&_End_& End MetaData";

            std::string contentHead;
                

            //只读取前8kb，如果没有找到MetaData则代表该文件不存在MetaData或文件以损坏。
            contentHead = ReadKiloBytes(8);

            FSLogCallback(std::format("[GwongDongFileSystem] ExtractMetaData - 前8KB内容:{}", contentHead), Plugin_Logs::logLevel::info, true);

            auto startPos = std::string::npos;
            auto endPos = std::string::npos;

            startPos = contentHead.find(metaDataStart);
            endPos = contentHead.find(metaDataEnd);

            if(startPos == std::string::npos || endPos == std::string::npos || endPos < startPos)
            {
                FSLogCallback("[GwongDongFileSystem] 找不到MetaData。", Plugin_Logs::logLevel::info, true);
                return "";
            }

            return contentHead.substr(startPos + metaDataStart.size(), endPos - startPos - metaDataStart.size());
        }

        //OPERATOR

        void RemoveFile() const override
        {
            if(!Exists())
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::info, true);
                return;
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
        FileMetaData _metaData;
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
        -> std::shared_ptr<ITotalFileOperator>
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto iter = std::find_if(baseFileArray.begin(), baseFileArray.end(), [&](const auto& TotalFileOperator){
                return TotalFileOperator->GetPathStr() == path.string();
            });

            if(iter == baseFileArray.end())
            {
                return nullptr;
            }

            return *iter;
        }

        auto NGetFile(const std::string_view name) const
        -> std::shared_ptr<ITotalFileOperator>
        {
            std::lock_guard<std::mutex> lock(mutex);
            auto iter = std::find_if(baseFileArray.begin(), baseFileArray.end(), [&](const auto& TotalFileOperator){
                return TotalFileOperator->GetFileName() == name;
            });

            if(iter == baseFileArray.end())
            {
                return nullptr;
            }

            return *iter;
        }

    public:


        //TODO:NMakeFile重载的命名随机化。
        template<typename Ty, typename ...Args>
        fs::path NMakeFile(Args&&... args)
        {
            if constexpr (!std::is_base_of_v<ITotalFileOperator, Ty>)
            {
                FSLogCallback("[GwongDongFileSystem] T必须继承于ITotalFileOperator。", Plugin_Logs::logLevel::err, true);
                return nullptr;
            }

            //MetaData在这里初始化。
            FileMetaData metaData;
            metaData.InitCreateTime();
            metaData.SetModifyTime();

            auto ptr = std::make_shared<Ty>(std::forward<Args>(args)..., std::move(metaData));
            Ty* rawPtr = ptr.get();

            {
                std::lock_guard<std::mutex> lock(mutex);
                std::ofstream outFile(rawPtr->GetPathStr(), std::ios::out);

                outFile.flush();
                outFile.close();
                
                baseFileArray.push_back(std::move(ptr));
                FSLogCallback(std::format("[GwongDongFileSystem] 文件创建成功。\n路径：{}", baseFileArray.back()->GetPathStr()), Plugin_Logs::logLevel::info, true);
                
                return baseFileArray.back()->GetPathStr();
            }
            
        }

        fs::path NMakeFile(fs::path& path, std::string_view name, std::string_view fileNameExtension)
        {
            FSLogCallback("[GwongDongFileSystem] 正在创建文件。", Plugin_Logs::logLevel::info, true);

            std::string finalName;

            
            if(path.empty())
            {
                FSLogCallback("[GwongDongFileSystem] path is empty, will create file at this directory.", Plugin_Logs::logLevel::info, true);
                path = fs::current_path();
            }

            if(fs::exists(path) && !fs::is_directory(path))
            {
                FSLogCallback("[GwongDongFileSystem] path is not a directory, will create file at this directory.", Plugin_Logs::logLevel::warn, true);
                path = path.parent_path();
                
            }

            if(name.empty())
            {
                FSLogCallback("[GwongDongFileSystem] name is empty, using random name.", Plugin_Logs::logLevel::warn, true);
                
                {
                    std::lock_guard<std::mutex> lock(mutex);
                    
                    finalName = []() {
                        auto now = std::chrono::system_clock::now();
                        auto now_mill = std::chrono::time_point_cast<std::chrono::milliseconds>(now);
                        auto timestamp = now_mill.time_since_epoch().count();

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

            FSLogCallback(std::format("[GwongDongFileSystem] 正在创建文件。\n路径：{}", path.string()), Plugin_Logs::logLevel::info, true);

            //MetaData在这里初始化。
            FileMetaData metaData;
            metaData.InitCreateTime();
            metaData.SetModifyTime();

    
            {
                std::lock_guard<std::mutex> lock(mutex);
                
                std::ofstream outFile(path, std::ios::out);
                outFile.flush();
                outFile.close();
                auto ptr = std::make_shared<FileObject>(std::forward<fs::path>(path), metaData);

                baseFileArray.emplace_back(std::move(ptr));
                FSLogCallback(std::format("[GwongDongFileSystem] 文件创建成功。\n路径：{}", baseFileArray.back()->GetPathStr()), Plugin_Logs::logLevel::info, true);
                return path;
            }

        }
        
        void AddFile(std::shared_ptr<ITotalFileOperator> file)
        {
            std::lock_guard<std::mutex> lock(mutex);
            baseFileArray.emplace_back(std::move(file));
        }

        void AddFile(fs::path& path)
        {
            std::lock_guard<std::mutex> lock(mutex);
            baseFileArray.emplace_back(std::make_shared<FileObject>(std::forward<fs::path>(path)));
        }

        void InputBase(fs::path& path, std::string_view content, WriteMode mode) const
        {
            auto file = NGetFile(path);

            if(file == nullptr)
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::warn, true);
                return;
            }

            file->WriteFile(content, mode);

            FSLogCallback(std::format("[GwongDongFileSystem] 文件写入成功。\n路径：{}", path.string()), Plugin_Logs::logLevel::info, true);
        }

        void NRenameFile(const fs::path& path, std::string_view newName) 
        {
            auto file = NGetFile(path);

            if(file == nullptr)
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::warn, true);
                return;
            }

            file->RenameTo(newName);
        }
        
        void NDeleteFile(const fs::path& path)
        {
            auto file = NGetFile(path);

            if(file == nullptr)
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::warn, true);
                return;
            }

            file->RemoveFile();

            FSLogCallback(std::format("[GwongDongFileSystem] 文件删除成功。\n路径：{}", path.string()), Plugin_Logs::logLevel::info, true);
        }
        
        void NCopyFile(const fs::path& from, const fs::path& to) const
        {
            auto file = NGetFile(from);

            if(file == nullptr)
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::warn, true);
                return;
            }

            file->Copy(to);

            FSLogCallback(std::format("[GwongDongFileSystem] 复制文件:{} -> {}。", from.string(), to.string()), Plugin_Logs::logLevel::info, true);
        }
        
        void NMoveFile(const fs::path& from, const fs::path& to) const
        {
            auto file = NGetFile(from);

            if(file == nullptr)
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::warn, true);
                return;
            }

            file->MoveTo(to);

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

        auto& NGetFileArray() const noexcept
        {
            return baseFileArray;
        }

        void SetMetaData(const fs::path& path, const FileMetaData& metaData)
        {
            auto file = NGetFile(path);

            if(file == nullptr)
            {
                FSLogCallback("[GwongDongFileSystem] 文件不存在。", Plugin_Logs::logLevel::warn, true);
                return;
            }

            file->WriteFile("", WriteMode::Append);
        }

    private:
        std::vector<std::shared_ptr<ITotalFileOperator>> baseFileArray;

    };


}

namespace GDfs = GwongDongFileSystem;