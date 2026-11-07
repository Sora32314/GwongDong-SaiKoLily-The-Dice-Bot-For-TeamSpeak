#pragma once

#include <span>
#include <array>
#include <bit>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <random>
#include <chrono>
#include <format>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <variant>
#include <vector>
#include <fstream>
#include <optional>
#include <algorithm>
#include <filesystem>
#include <functional>
#include <loggings.hpp>

#define DEBUG_VERSION_V1 -1
#define DEBUG_V1 static_cast<std::byte>(DEBUG_VERSION_V1)

namespace fs = std::filesystem;
using Path = std::filesystem::path;
using FileTime = std::chrono::system_clock::time_point;

#define _DEBUG

namespace GwongDongFileSystem
{
    //日志记录
    using LogCallBack = std::function<void(const std::string&, Plugin_Logs::logLevel, bool nowFlush)>;

    extern LogCallBack FSLogCallback;
    void SetLogCallback(LogCallBack callback);    

    inline constexpr std::array<std::byte, 3> METADATA_START_SIGN{std::byte{0xF8}, std::byte{0xFF}, std::byte{0xFD}};
    inline constexpr std::array<std::byte, 3> METADATA_END_SIGN{std::byte{0xFD}, std::byte{0xFF}, std::byte{0xF8}};



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
            _metadata_version = std::byte{0};
        }

        //UpdateMetaData：按传入参数【修改】类内MetaData，不负责返回序列化文本。
        //仅更新有值(has_value)的字段，未传入的字段保持原样；
        //默认每次更新视为一次内容修改并刷新_modifyTime，
        //如需只改字段而不刷新修改时间，请显式传入 refreshModifyTime = false。
        //【获取】序列化后的MetaData文本，请调用无副作用的 GetMetaData()。
        virtual void UpdateMetaData(
            std::optional<std::string> name              = std::nullopt,
            std::optional<std::string> owner             = std::nullopt,
            std::optional<std::string> extension         = std::nullopt,
            std::optional<FileType>    fileType          = std::nullopt,
            std::optional<std::string> informationLeave  = std::nullopt,
            std::optional<bool>        refreshModifyTime = true)
        {
            if(name.has_value())
            {
                SetName(*name);
            }

            if(owner.has_value())
            {
                SetOwner(*owner);
            }

            if(extension.has_value())
            {
                SetExtension(*extension);
            }

            if(fileType.has_value())
            {
                SetFileType(*fileType);
            }

            if(informationLeave.has_value())
            {
                SetInformationLeave(*informationLeave);
            }

            if(refreshModifyTime.value_or(true))
            {
                SetModifyTime();
            }
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

        virtual void SetModifyTime(const FileTime& time)
        {
            _modifyTime = time;
        }

        virtual void SetName(std::string_view name)
        {
            _name = std::move(name);
        }

        virtual void SetInformationLeave(std::string_view informationLeave)
        {
            _informationLeave = std::move(informationLeave);
        }

        virtual void SetMetaDataVersion(std::byte version)
        {
            _metadata_version = version;
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

        virtual std::byte GetMetaDataVersion() const
        {
            return _metadata_version;
        }
    
        //需要先刷新 _modifyTime 时请先调用 SetModifyTime()。
        virtual std::string GetMetaData() const
        {
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
                throw std::runtime_error("[GwongDongFileSystem] FileType未定义！");
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
        private: 
        std::string _name;
        FileTime _createTime;
        FileTime _modifyTime;
        std::string _owner;
        std::string _informationLeave;
        std::string _extension;
        FileType _fileType;

        //neg:测试用版本 pos:正式版本
        std::byte _metadata_version = DEBUG_V1;
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

        //从FileMetaData序列化二进制元数据
        static std::vector<std::byte> SerializeMetaDataBinary(const FileMetaData& metaData)
        {
            using MetaField = std::variant<
                                            std::string,       // name / owner / informationLeave / extension
                                            FileTime,          // createTime / modifyTime
                                            FileType,          // fileType
                                            std::byte          // metadataVersion
                                        >;
            
            //序列化为二进制格式
            std::vector<std::byte> string_stream;

            

            //起始字段。
            string_stream.insert(string_stream.end(), {std::byte{0xF8}, std::byte{0xFF}, std::byte{0xFD}});
            


            auto serializeField = [&string_stream](const MetaField& field) -> void
            {
                std::visit([&string_stream](auto&& arg) -> void
                {
                    using T = std::decay_t<decltype(arg)>;
                    if constexpr (std::is_same_v<T, std::string>)
                    {
                        string_stream.insert(string_stream.end(),
                                             reinterpret_cast<const std::byte*>(arg.data()),
                                             reinterpret_cast<const std::byte*>(arg.data()) + arg.size());
                    }
                    else if constexpr (std::is_same_v<T, FileTime>)
                    {
                        auto time = std::chrono::duration_cast<std::chrono::nanoseconds>(arg.time_since_epoch()).count();
                        auto time_bytes = reinterpret_cast<const std::byte*>(&time);
                        string_stream.insert(string_stream.end(), time_bytes, time_bytes + sizeof(time));
                    }
                    else if constexpr (std::is_same_v<T, FileType>)
                    {
                        auto type = static_cast<std::underlying_type_t<FileType>>(arg);
                        auto type_bytes = reinterpret_cast<const std::byte*>(&type);
                        string_stream.insert(string_stream.end(), type_bytes, type_bytes + sizeof(type));
                    }
                    else if constexpr (std::is_same_v<T, std::byte>)
                    {
                        string_stream.push_back(arg);
                    }
                }, field);
            };

            auto NameField = [&string_stream](const std::string& name)
                -> void
            {
                string_stream.insert(string_stream.end(),
                                     reinterpret_cast<const std::byte*>(name.data()),
                                     reinterpret_cast<const std::byte*>(name.data()) + name.size());
                string_stream.push_back(static_cast<std::byte>('\0'));
            };


            serializeField(metaData.GetName());
            serializeField(metaData.GetCreateTime());
            serializeField(metaData.GetModifyTime());
            serializeField(metaData.GetOwner());
            serializeField(metaData.GetInformationLeave());
            serializeField(metaData.GetExtension());
            serializeField(metaData.GetFileType());
            serializeField(metaData.GetMetaDataVersion());
            
            string_stream.insert(string_stream.end(), {std::byte{0xFD}, std::byte{0xFF}, std::byte{0xF8}});
            
            //最终确定长度
            std::byte sizeMetaData[2] = {
                static_cast<std::byte>((string_stream.size() + 2 >> 8) & 0xFF),
                static_cast<std::byte>(string_stream.size() + 2 & 0xFF)
            }; 

            string_stream.insert(string_stream.begin() + 3, sizeMetaData, sizeMetaData + sizeof(sizeMetaData));
            
            return string_stream;
        };

        //反序列化二进制元数据到FileMetaData
        static FileMetaData DeserializeMetaDataBinary(const std::vector<std::byte>& data)
        {
            //二进制格式（TLV）：
            //  Frame = StartSign(3B: F8 FF FD) | FrameLength(2B, 大端, 含全部开销)
            //        | TLV... | EndSign(3B: FD FF F8)
            //  TLV   = TLVHeader(2B, 大端) | Content(Length B)
            //  TLVHeader 高4位 = tag，低12位 = Length（最大 4095）
            //tag分配：0x0 保留/填充,0x1 Name,0x2 CreateTime,0x3 ModifyTime,0x4 Owner,
            //        0x5 InformationLeave,0x6 Extension,0x7 FileType,0x8 MetaDataVersion,
            //        0x9~0xE 扩展,0xF 转义/扩展保留
            //编码：字符串 = 原始UTF-8字节（不带'\0'）；时间 = int64纳秒（8B大端）；
            //      FileType / MetaDataVersion = 1B。
            //未知tag按Length静默跳过，保证向前兼容。
            return [&]()
                -> FileMetaData
            {
                FileMetaData metaData;

                std::span<const std::byte> payLoad(data);
            

                constexpr std::size_t startSigSize    = 3;
                constexpr std::size_t lengthFieldSize = 2;
                constexpr std::size_t endSigSize      = 3;
                constexpr std::size_t headerSize      = startSigSize + lengthFieldSize;
                constexpr std::size_t overhead        = headerSize + endSigSize;

                if(payLoad.size() < overhead)
                {
                    FSLogCallback("[GwongDongFileSystem] DESERIALIZE_METADATA_BINARY：文件不存在MetaData，文件可能已损坏或你误打开了非该文件系统所管理的文件。", Plugin_Logs::logLevel::warn, true);
                    return metaData;
                }

                if(!std::ranges::equal(payLoad.first(startSigSize), METADATA_START_SIGN))
                {
                    FSLogCallback("[GwongDongFileSystem] DESERIALIZE_METADATA_BINARY：MetaData起始签名不匹配，文件可能已损坏或你误打开了非该文件系统所管理的文件。", Plugin_Logs::logLevel::warn, true);
                    return metaData;
                }

                //长度字段为整帧总长（含前后签名与长度字段本身），大端。
                const std::uint16_t frameLength = static_cast<std::uint16_t>(std::to_integer<std::uint16_t>(payLoad[3]) << 8 |
                                               std::to_integer<std::uint16_t>(payLoad[4]));

                if(frameLength < overhead || frameLength > payLoad.size())
                {
                    FSLogCallback(std::format("[GwongDongFileSystem] DESERIALIZE_METADATA_BINARY：MetaData长度字段为{}字节，但数据实际有{}字节，请检查文件。", frameLength, payLoad.size()), Plugin_Logs::logLevel::warn, true);
                    return metaData;
                }

                //只解析长度字段覆盖的范围，忽略帧后可能存在的文件正文。
                const std::span<const std::byte> frame = payLoad.first(frameLength);

                if(!std::ranges::equal(frame.last(endSigSize), METADATA_END_SIGN))
                {
                    FSLogCallback("[GwongDongFileSystem] DESERIALIZE_METADATA_BINARY：MetaData结束签名不匹配，文件可能已损坏或你误打开了非该文件系统所管理的文件。", Plugin_Logs::logLevel::warn, true);
                    return metaData;
                }

                payLoad = frame.subspan(headerSize, frameLength - overhead);

                //空字段（长度为0）时 span::data() 不保证非空，统一交给这个辅助函数兜底。
                auto MemoryCast_Span_to_String_View = [](std::span<const std::byte> byteStream) -> std::string_view
                {
                    if(byteStream.empty())
                    {
                        return {};
                    }
                    return std::string_view(reinterpret_cast<const char*>(byteStream.data()), byteStream.size());
                };

                //状态解析状态机
                for(std::size_t usedByte = 0; usedByte < payLoad.size() && !payLoad.empty();)
                {
                    if(usedByte + 2 > payLoad.size())
                    {
                        FSLogCallback("[GwongDongFileSystem] DESERIALIZE_METADATA_BINARY：MetaData字段头不完整，请检查文件。", Plugin_Logs::logLevel::warn, true);
                        return metaData;
                    }

                    //TLV头 2B 大端：高4位 tag，低12位 Content 长度。
                    const std::uint16_t tlvHeader =
                        static_cast<std::uint16_t>(std::to_integer<std::uint16_t>(payLoad[usedByte]) << 8 |
                                                   std::to_integer<std::uint16_t>(payLoad[usedByte + 1]));

                    const std::uint8_t  tag        = static_cast<std::uint8_t>(tlvHeader >> 12);
                    const std::uint16_t tag_length = static_cast<std::uint16_t>(tlvHeader & 0x0FFFu);

                    usedByte += 2;     //跳过 tag(4bit) + length(12bit)

                    if(usedByte + tag_length > payLoad.size())
                    {
                        FSLogCallback(std::format("[GwongDongFileSystem] DESERIALIZE_METADATA_BINARY：MetaData字段(tag=0x{:X})声明长度为{}字节，但剩余数据不足，请检查文件。", tag, tag_length), Plugin_Logs::logLevel::warn, true);
                        return metaData;
                    }


                    switch(tag)
                    {
                    case 0x0:
                        //保留/填充：按自描述长度跳过
                        break;
                    case 0x1:
                        //Name
                        metaData.SetName(MemoryCast_Span_to_String_View(payLoad.subspan(usedByte, tag_length)));
                        break;
                    case 0x2:
                    case 0x3:
                    {
                        //CreateTime / ModifyTime：8B 大端 int64 纳秒
                        if(tag_length != sizeof(std::uint64_t))
                        {
                            FSLogCallback(std::format("[GwongDongFileSystem] DESERIALIZE_METADATA_BINARY：时间字段(tag=0x{:X})长度应为8字节，实际为{}字节，请检查文件。", tag, tag_length), Plugin_Logs::logLevel::warn, true);
                            return metaData;
                        }

                        std::uint64_t rawTime = 0;
                        std::memcpy(&rawTime, payLoad.subspan(usedByte, tag_length).data(), sizeof(rawTime));
                        rawTime = std::byteswap(rawTime);
                        const std::int64_t nanoseconds = std::bit_cast<std::int64_t>(rawTime);
                        const FileTime time(std::chrono::duration_cast<FileTime::duration>(std::chrono::nanoseconds{nanoseconds}));

                        if(tag == 0x2)
                        {
                            metaData.SetCreateTime(time);
                        }
                        else
                        {
                            metaData.SetModifyTime(time);
                        }
                        break;
                    }
                    case 0x4:
                        //Owner
                        metaData.SetOwner(MemoryCast_Span_to_String_View(payLoad.subspan(usedByte, tag_length)));
                        break;
                    case 0x5:
                        //InformationLeave
                        metaData.SetInformationLeave(MemoryCast_Span_to_String_View(payLoad.subspan(usedByte, tag_length)));
                        break;
                    case 0x6:
                        //Extension
                        //显式构造 string，避免命中 fs::path 重载
                        metaData.SetExtension(std::string(MemoryCast_Span_to_String_View(payLoad.subspan(usedByte, tag_length))));
                        break;
                    case 0x7:
                    {
                        //FileType：1B
                        if(tag_length != 1)
                        {
                            FSLogCallback(std::format("[GwongDongFileSystem] DESERIALIZE_METADATA_BINARY：FileType字段长度应为1字节，实际为{}字节，请检查文件。", tag_length), Plugin_Logs::logLevel::warn, true);
                            return metaData;
                        }

                        const std::uint8_t rawType = std::to_integer<std::uint8_t>(payLoad[usedByte]);
                        metaData.SetFileType(rawType <= static_cast<std::uint8_t>(FileType::Other)
                                                 ? static_cast<FileType>(rawType)
                                                 : FileType::Unknown);
                        break;
                    }
                    case 0x8:
                    {
                        //MetaDataVersion：1B
                        if(tag_length != 1)
                        {
                            FSLogCallback(std::format("[GwongDongFileSystem] DESERIALIZE_METADATA_BINARY：MetaDataVersion字段长度应为1字节，实际为{}字节，请检查文件。", tag_length), Plugin_Logs::logLevel::warn, true);
                            return metaData;
                        }

                        metaData.SetMetaDataVersion(payLoad[usedByte]);
                        break;
                    }
                    default:
                        //0x9~0xE 扩展、0xF 转义/扩展保留：按自描述长度静默跳过，保证向前兼容。
                        break;
                    }
                    
                    usedByte += tag_length;
                }

                
                return metaData;
            }();
        }

        //从文本反序列化
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
    

    private:


    };



    class FileObject : public ITotalFileOperator
    {
    public:
        //【新建并落盘】构造：创建/覆写磁盘文件，并把可选的 metaData 字段合并进文件头后写入。
        //metaData 仅【借用】：内部同步拷贝字段到本对象 _metaData，不持有、不延长外部对象生命周期；传 nullptr 表示使用默认 MetaData。
        FileObject(const Path& path, const FileMetaData* metaData = nullptr)
            : _path(path)
        {
            InitializeFileOnDisk(metaData);


            #ifdef _DEBUG
                FSLogCallback(std::format("[GwongDongFileSystem] MetaData:{}", _metaData.GetMetaData()), Plugin_Logs::logLevel::info, true);
            #endif
        }

        //【打开既有】构造：不创建/覆写磁盘文件，直接把给定的 FileMetaData 值载入内存 _metaData。
        //调用规约（避免与上面的“新建”重载撞车）：
        //  传 const FileMetaData*（&md / nullptr）→ 新建并落盘（命中上一构造）；
        //  传 FileMetaData 值（md）              → 打开既有、仅载入内存。
        //注意：外部若临时构造 FileMetaData 想作为“新建文件”头部字段，务必取地址传 &md；
        //      误传值会命中本“打开既有”构造，导致文件不被创建。
        FileObject(const Path& path, FileMetaData metaData)
            : _path(path), _metaData(std::move(metaData))
        {}

        mutable std::mutex _mutex;
        
    public:
        //INIT
        //metaData 为可选的【借用】字段源：仅把外部 name/owner/extension/fileType/informationLeave 合并进本对象 _metaData 后写入文件头，不持有所有权；nullptr 表示使用默认 MetaData。
        void InitializeFileOnDisk(const FileMetaData* metaData = nullptr)
        {
            if(!fs::exists(_path.parent_path()))
            {
                fs::create_directories(_path.parent_path());
            }

            std::ofstream file(_path, std::ios::out | std::ios::trunc);
            if(!file)
            {
                FSLogCallback(std::format("[GwongDongFileSystem] 文件创建失败。\n路径：{}", _path.string()), Plugin_Logs::logLevel::err, true);
                return;
            }


            _metaData.InitCreateTime();
            _metaData.SetModifyTime();
            
            if(metaData != nullptr)
            {

                _metaData.UpdateMetaData(
                    metaData->GetName(),
                    metaData->GetOwner(),
                    std::nullopt,
                    metaData->GetFileType(),
                    metaData->GetInformationLeave(),
                    false //不刷新修改时间
                );
            }

            if(!_path.extension().empty())
            {
                _metaData.SetExtension(_path);
            }

            std::string metaDataStr = _metaData.GetMetaData();

            file.write(metaDataStr.data(), metaDataStr.size());
        }


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

                std::lock_guard<std::mutex> lock(_mutex);

                if(MetaDataSerializer::HasMetaData(res))
                {
                    auto metaData = MetaDataSerializer::DeserializeMetaDataText(res);

                    #ifdef _DEBUG
                        FSLogCallback(std::format("[GwongDongFileSystem] Write::Override:MetaData:{}", res), Plugin_Logs::logLevel::info, true);
                    #endif

                    _metaData = std::move(metaData);

                    _metaData.SetModifyTime();
                    finalContent = _metaData.GetMetaData() + std::string(content);
                }
                else
                {
                    finalContent = _metaData.GetMetaData() + std::string(content);
                }
                
                std::ofstream file(_path, std::ios::out | std::ios::trunc);
                if(!file)
                {
                    FSLogCallback(std::format("[GwongDongFileSystem] 文件写入失败。\n路径：{}", _path.string()), Plugin_Logs::logLevel::err, true);
                    return;
                }


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

                std::lock_guard<std::mutex> lock(_mutex);

                //写入源信息
                if(MetaDataSerializer::HasMetaData(chunk_bytes))
                {
                    auto cachedMetaData = MetaDataSerializer::DeserializeMetaDataText(chunk_bytes);

                    std::string chunk_without_metadata = chunk_bytes.substr(end_pos, chunk_bytes.size());

                    _metaData = std::move(cachedMetaData);

                    _metaData.SetModifyTime();
                    chunk_bytes = _metaData.GetMetaData() + chunk_without_metadata;

                    {
                        std::fstream file(_path, std::ios::in | std::ios::out);
                        if(!file)
                        {
                            FSLogCallback(std::format("[GwongDongFileSystem] 文件写入失败。\n路径：{}", _path.string()), Plugin_Logs::logLevel::err, true);
                            return;
                        }

                        file.seekp(0, std::ios::beg);
                        file.write(chunk_bytes.data(), chunk_bytes.size());

                        file.flush();
                    }
                }
                else
                {
                    _metaData.SetModifyTime();
                    auto cachedMetaData = _metaData.GetMetaData();

                    {
                        std::fstream file(_path, std::ios::in | std::ios::out);
                        if(!file)
                        {
                            FSLogCallback(std::format("[GwongDongFileSystem] 文件写入失败。\n路径：{}", _path.string()), Plugin_Logs::logLevel::err, true);
                            return;
                        }

                        file.seekp(0, std::ios::beg);
                        file.write(cachedMetaData.data(), cachedMetaData.size());

                        file.flush();
                    }

                }

                //写入其他信息
                std::ofstream file(_path, std::ios::out | std::ios::ate);
                if(!file)
                {
                    FSLogCallback(std::format("[GwongDongFileSystem] 文件写入失败。\n路径：{}", _path.string()), Plugin_Logs::logLevel::err, true);
                    return;
                }

                file.write(content.data(), content.size());

                file.flush();
                break;
            }
            case WriteMode::Prepend:
            {
                auto chunk_bytes = ReadKiloBytes(8);
                const std::string metaDataEnd = "&_End_& End MetaData";
                auto end_pos = chunk_bytes.find(metaDataEnd) + metaDataEnd.size();

                std::lock_guard<std::mutex> lock(_mutex);

                //写入源信息
                if(MetaDataSerializer::HasMetaData(chunk_bytes))
                {
                    auto cachedMetaData = MetaDataSerializer::DeserializeMetaDataText(chunk_bytes);

                    std::string chunk_without_metadata = chunk_bytes.substr(end_pos, chunk_bytes.size());

                    _metaData = std::move(cachedMetaData);

                    _metaData.SetModifyTime();
                    chunk_bytes = _metaData.GetMetaData() + chunk_without_metadata;

                    {
                        std::fstream file(_path, std::ios::in | std::ios::out);
                        if(!file)
                        {
                            FSLogCallback(std::format("[GwongDongFileSystem] 文件写入失败。\n路径：{}", _path.string()), Plugin_Logs::logLevel::err, true);
                            return;
                        }

                        file.seekp(0, std::ios::beg);
                        file.write(chunk_bytes.data(), chunk_bytes.size());

                        file.flush();
                    }
                }
                else
                {
                    _metaData.SetModifyTime();
                    auto cachedMetaData = _metaData.GetMetaData();

                    {
                        std::fstream file(_path, std::ios::in | std::ios::out);
                        if(!file)
                        {
                            FSLogCallback(std::format("[GwongDongFileSystem] 文件写入失败。\n路径：{}", _path.string()), Plugin_Logs::logLevel::err, true);
                            return;
                        }

                        file.seekp(0, std::ios::beg);
                        file.write(cachedMetaData.data(), cachedMetaData.size());

                        file.flush();
                    }

                }

                //写入其他信息
                std::ofstream file(_path, std::ios::out | std::ios::ate);
                if(!file)
                {
                    FSLogCallback(std::format("[GwongDongFileSystem] 文件写入失败。\n路径：{}", _path.string()), Plugin_Logs::logLevel::err, true);
                    return;
                }

                
                file.seekp(end_pos, std::ios::beg);
                file.write(content.data(), content.size());

                file.flush();
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
                std::ifstream inFile(_path, std::ios::in);
                std::stringstream ss;
                ss << inFile.rdbuf();
                content = ss.str();
            }

            return content.substr(offset, length);
        }

        std::string ReadKiloBytes(size_t bytes) const override
        {
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
                return {};
            }

            auto FileOperatorPtr = std::make_shared<FileObject>(std::forward<Args>(args)...);
            {
                std::lock_guard<std::mutex> lock(mutex);
                baseFileArray.push_back(std::move(FileOperatorPtr));
                FSLogCallback(std::format("[GwongDongFileSystem] 文件创建成功。\n路径：{}", baseFileArray.back()->GetPathStr()), Plugin_Logs::logLevel::info, true);

                return baseFileArray.back()->GetPathStr();
            }

            /*
            //MetaData在这里初始化。
            FileMetaData metaData;
            metaData.InitCreateTime();
            metaData.SetModifyTime();

            auto ptr = std::make_shared<Ty>(std::forward<Args>(args)..., std::move(metaData));
            Ty* rawPtr = ptr.get();

            {
                std::lock_guard<std::mutex> lock(mutex);
                std::ofstream outFile(rawPtr->GetPathStr(), std::ios::out);
                if(!outFile)
                {
                    FSLogCallback(std::format("[GwongDongFileSystem] 文件创建失败。\n路径：{}", rawPtr->GetPathStr()), Plugin_Logs::logLevel::err, true);
                    return {};
                }


                outFile.flush();
                outFile.close();
                
                baseFileArray.push_back(std::move(ptr));
                FSLogCallback(std::format("[GwongDongFileSystem] 文件创建成功。\n路径：{}", baseFileArray.back()->GetPathStr()), Plugin_Logs::logLevel::info, true);
                
                return baseFileArray.back()->GetPathStr();
            }
            */
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

            auto FileOperatorPtr = std::make_shared<FileObject>(std::forward<fs::path>(path), nullptr);
            {
                std::lock_guard<std::mutex> lock(mutex);
                baseFileArray.push_back(std::move(FileOperatorPtr));
                FSLogCallback(std::format("[GwongDongFileSystem] 文件创建成功。\n路径：{}", baseFileArray.back()->GetPathStr()), Plugin_Logs::logLevel::info, true);

                return baseFileArray.back()->GetPathStr();
            }

            /*
            //MetaData在这里初始化。
            FileMetaData metaData;
            metaData.InitCreateTime();
            metaData.SetModifyTime();

    
            {
                std::lock_guard<std::mutex> lock(mutex);
                
                std::ofstream outFile(path, std::ios::out);
                if(!outFile)
                {
                    FSLogCallback(std::format("[GwongDongFileSystem] 文件创建失败。\n路径：{}", path.string()), Plugin_Logs::logLevel::err, true);
                    return {};
                }

                outFile.flush();
                outFile.close();
                auto ptr = std::make_shared<FileObject>(std::forward<fs::path>(path), metaData);

                baseFileArray.emplace_back(std::move(ptr));
                FSLogCallback(std::format("[GwongDongFileSystem] 文件创建成功。\n路径：{}", baseFileArray.back()->GetPathStr()), Plugin_Logs::logLevel::info, true);
                return path;
            }

            */
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