#include <CommandCore.hpp>
#include <unordered_map>
#include <mutex>

/*
    可能使用mutex保证并行支持，虽然其他模块暂时尚未支持多线程，但是不排除后续支持多线程修改可能。
*/




namespace Command_Core
{
    static CC_LogCallback cc_logCallback = nullptr;
    //设置Log封装回调函数
    void SetLogCallback(CC_LogCallback callback)
    {
        cc_logCallback = callback;
    }

    class CommandRegistryImpl final : public ICommandRegistry
    {
    public:
        void RegisterHandler(std::unique_ptr<ICommandHandler> handler) override
        {
            std::lock_guard<std::mutex> lock(mutex_);
            std::string_view name = handler->GetName();
            auto aliasList = handler->GetAlias();
            handlers.emplace(name, std::move(handler));
            
            
            //注册别名
            for (auto alias : aliasList)
            {
                aliases.emplace(alias, name);
            }
        }

        ICommandHandler* FindHandler(std::string_view command) const override
        {
            std::lock_guard<std::mutex> lock(mutex_);

            //先尝试在命令存储中寻找
            auto iter = handlers.find(command);
            auto aliasIter = aliases.find(command);
            if(iter != handlers.end())
            {
                return iter->second.get();
            }

            //再尝试从别名中寻找后去主存储中判断别名是否存在对应的命令，之后返回
            if(aliasIter != aliases.end())
            {
                auto iter_from_alias = handlers.find(aliasIter->second);
                if(iter_from_alias != handlers.end())
                {
                    return iter_from_alias->second.get();
                }
            }

            return nullptr;
        }

    private:
        mutable std::mutex mutex_;
        std::map<std::string_view, std::unique_ptr<ICommandHandler>, std::less<>> handlers;
        std::map<std::string_view, std::string_view, std::less<>> aliases;
    };

    class CommandCheckerImpl final : public ICommandChecker
    {
    public:
        CommandCheckerImpl(ICommandRegistry& _registry) : registry(_registry) {}

        void SetResultCallback(std::shared_ptr<IResultCallback> callback) override
        {
            resultCallback = callback;
        }

        //检测命令逻辑
        bool ProcessMessage
        (
            std::string_view message,
            InfoFetcher Info,
            MessageTarget messageTarget
        ) override
        {
            //验证消息格式
            if(message.empty() || (message[0] != '!' && message[0] != '.' && message[0] != '>'))
            {
                //检测到输入，但判断为非命令
                return false;
            }

            //提取命令以及参数
            std::string_view command_view = message.substr(1);
            std::vector<std::string_view> tokens;
            TokenizeCommand(command_view, tokens);

            if(tokens.empty())
            {
                //非有效命令
                return false;
            }

            //查找命令验证器
            auto* handler = registry.FindHandler(tokens[0]);
            if(!handler)
            {
                cc_logCallback(std::format("未找到命令：{}", tokens[0]), Plugin_Logs::logLevel::warn, false);
                return false;
            }


            //创建上下文
            CommandContextImpl context
            (
                std::string(message),
                Info,
                messageTarget,
                tokens,
                resultCallback
            );

            //执行命令
            try
            {
                handler->Execute(context);
                cc_logCallback(std::format("命令执行：{}", std::string(tokens[0])), Plugin_Logs::logLevel::info, false);
                return true;
            }
            catch (const std::exception& err)
            {
                cc_logCallback(std::format("命令：{}，执行错误！\n错误信息：{}", std::string(tokens[0]), std::string(err.what())), Plugin_Logs::logLevel::err, false);
                return true;
            }

        }
    private:
        //分词器，防止创建多余字符串

        //TODO:改进分词器-State_2
        void TokenizeCommand(std::string_view inputs, std::vector<std::string_view>& tokens)
        {
            tokens.clear();
            size_t start = 0;
            size_t end = 0;

            auto isSpace = [](char character) { return std::isspace(static_cast<unsigned char>(character)); };

            while (start < inputs.size())
            {
                while (start < inputs.size() && isSpace(inputs[start]))
                {
                    start++;
                }

                if (start >= inputs.size())
                {
                    break;
                }

                end = start;
                char firstChar = inputs[start];

                if (firstChar == '"' || firstChar == '\'')
                {
                    char quoteChar = firstChar;
                    end++;

                    while (end < inputs.size() && inputs[end] != quoteChar)
                    {
                        end++;
                    }

                    if (end < inputs.size())
                    {
                        end++; 
                    }
                    
                    //如果想去除引号，可以使用 tokens.push_back(inputs.substr(start + 1, end - start - 2));
                    //现在去除引号
                    
                    tokens.push_back(inputs.substr(start + 1, end - start - 2));
                }
                else
                {
                    while (end < inputs.size() && !isSpace(inputs[end]))
                    {
                        if (inputs[end] == '"' || inputs[end] == '\'')
                        {
                            break;
                        }
                        end++;
                    }
                    tokens.push_back(inputs.substr(start, end - start));
                }
                
                start = end;
            }
        }
       

        class CommandContextImpl final : public ICommandContext
        {
        public:
            CommandContextImpl
            (
                std::string _rawCommand,
                InfoFetcher _user_info,
                MessageTarget _messageTarget,
                const std::vector<std::string_view>& _tokens,
                std::shared_ptr<IResultCallback> _callback
            ) :
            rawCommand(_rawCommand),
            user_info(_user_info),
            messageTarget(_messageTarget),
            tokens(_tokens),
            callback(_callback)
            {}

            void SendResult(const std::string& message, MessageTarget target) const override
            {
                if(callback)
                {
                    callback->SendResult(user_info.userID, user_info.channelID, user_info.serverID, target, message);
                }
            }

            ~CommandContextImpl() override = default;

            std::string_view GetRawCommand() const override
            {
                return rawCommand;
            }

            uint64 GetCallingUserID() const override
            {
                return user_info.userID;
            }

            uint64 GetChannelID() const override
            {
                return user_info.channelID;
            }

            uint64 GetServerID() const override
            {
                return user_info.serverID;
            }

            std::string_view GetCallingUserName() const override
            {
                if(user_info.UserName.has_value())
                    return user_info.UserName.value();
                return "";
            }

            std::string_view GetChannelName() const override
            {
                if(user_info.ChannelName.has_value())
                    return user_info.ChannelName.value();
                return "";
            }

            std::string_view GetServerName() const override
            {
                if(user_info.ServerName.has_value())
                    return user_info.ServerName.value();
                return "";
            }

            void Log(std::string_view message, Plugin_Logs::logLevel level, bool nowFlush) const override
            {
                cc_logCallback(std::format("回复日志：{}", message), level, nowFlush);
            }

            std::optional<std::string_view> GetParam(size_t index) const override
            {
                if(index + 1 < tokens.size())
                {
                    return tokens[index + 1];
                }
                return std::nullopt;
            }

            size_t GetParamCount() const override
            {
                return tokens.size() > 1 ? tokens.size() - 1 : 0;
            }

            MessageTarget GetMessageTarget() const override
            {
                return messageTarget;
            }

            std::unordered_map<std::string, ClientInfoPackage> GetAllUsersInChannel() const override
            {
                std::unordered_map<std::string, ClientInfoPackage> users;

                for(auto& [UUID, ClientInfo] : GetClientInfoStorage().GetClientCache()[user_info.serverID])
                {
                    if(ClientInfo.channelID == user_info.channelID)
                        users.try_emplace(UUID, ClientInfo);
                }

                cc_logCallback(std::format("获取频道内用户：{}人。", users.size()), Plugin_Logs::logLevel::info, false);
                return users;
            }

            std::unordered_map<std::string, ClientInfoPackage> GetAllUsersInServer() const override
            {
                std::unordered_map<std::string, ClientInfoPackage> users;

                for(auto& [UUID, userInfo] : GetClientInfoStorage().GetClientCache()[user_info.serverID])
                {
                    users.emplace(UUID, userInfo);
                }

                return users;
            }

            std::unordered_map<std::string, ClientInfoPackage> GetAllUsers() const override
            {
                std::unordered_map<std::string, ClientInfoPackage> users;
                for(auto& [ServerHandler, Pair] : GetClientInfoStorage().GetClientCache())
                {
                    for(auto& [UUID, ClientInfo] : Pair)
                    {
                        users.emplace(UUID, ClientInfo);
                    }
                }
                return users;
            }

            std::vector<std::string> GetUserNameByID(uint64 userID) const override
            {
                std::vector<std::string> userName;
                for (auto& [ServerHandler, Pair] : GetClientInfoStorage().GetClientCache())
                {
                    for (auto& [ClientID, ClientInfo] : Pair)
                    {
                        if (ClientInfo.clientID == userID)
                        {
                            userName.push_back(ClientInfo.nickName);
                        }
                    }
                }
                return userName;
            }

            std::vector<std::string> GetUserNameByID(uint64_t serverHandler, uint64 userID) const override
            {
                std::vector<std::string> userName;
                for (auto Pair : GetClientInfoStorage().GetClientCache()[serverHandler])
                {
                    if (Pair.second.clientID == userID)
                    {
                        userName.push_back(Pair.second.nickName);
                    }
                }

                return userName;
            }

            std::string_view GetUserUUID(uint64 userID) const override
            {
                for(auto& [UUID, ClientInfo] : GetClientInfoStorage().GetClientCache()[user_info.serverID])
                {
                    if(ClientInfo.clientID == userID)
                    {
                        return ClientInfo.uniqueID;
                    }
                }
                return "Unknow";
            }

            ClientInfoPackage GetUser(uint64 userID) const override
            {
                for(auto& [UUID, ClientInfo] : GetClientInfoStorage().GetClientCache()[user_info.serverID])
                {
                    if(ClientInfo.clientID == userID)
                    {
                        return ClientInfo;
                    }
                }

                cc_logCallback(std::format("未通过ID：{}在服务器：{}上查找到用户。", userID, user_info.ServerName.value_or("Unknow")), Plugin_Logs::logLevel::warn, false);
                return ClientInfoPackage{};
            }

            ClientInfoPackage GetUserByName(std::string_view name) const override
            {
                for(auto& [UUID, ClientInfo] : GetClientInfoStorage().GetClientCache()[user_info.serverID])
                {
                    if(ClientInfo.nickName == name)
                    {
                        return ClientInfo;
                    }
                }
                
                cc_logCallback(std::format("未通过昵称：{}在服务器：{}上查找到用户。", name, user_info.ServerName.value_or("Unknow")), Plugin_Logs::logLevel::warn, false);
                return ClientInfoPackage{};
            }

        private:
            std::string rawCommand;
            InfoFetcher user_info;
            std::vector<std::string_view> tokens;
            std::shared_ptr<IResultCallback> callback;
            MessageTarget messageTarget;
        };


    private:
        ICommandRegistry& registry;
        std::vector<std::string_view> tokens;
        std::shared_ptr<IResultCallback> resultCallback;
    };

    class ClientInfoStorage : public IClientInfoStorage
    {
    public:
        ClientInfoStorage(const ClientInfoStorage&) = delete;
        ClientInfoStorage& operator=(const ClientInfoStorage&) = delete;

        static ClientInfoStorage& GetInstance()
        {
            static ClientInfoStorage instance;
            return instance;
        }

        std::unordered_map<uint64_t, std::unordered_map<std::string, ClientInfoPackage>>& GetClientCache() 
        {
            return *clientCache;
        }

    private:
        ClientInfoStorage()
            : clientCache(std::make_unique<std::unordered_map<uint64_t, std::unordered_map<std::string, ClientInfoPackage>>>())
        {}
        //                              ServerConnectionHandler         UUID         ClientInfoPackage
        std::unique_ptr<std::unordered_map<uint64_t, std::unordered_map<std::string, ClientInfoPackage>>> clientCache;
    };

    IClientInfoStorage& GetClientInfoStorage()
    {
        return ClientInfoStorage::GetInstance();
    }

    // 工厂实现 - 确保内存分配/释放在同一模块
    std::unique_ptr<ICommandRegistry> CreateRegistry() {
        return std::make_unique<CommandRegistryImpl>();
    }

    std::unique_ptr<ICommandChecker> CreateCMDChecker(ICommandRegistry& registry)
    {
        return std::make_unique<CommandCheckerImpl>(registry);
    }


}
