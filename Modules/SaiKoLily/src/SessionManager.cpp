#include <SessionManager.hpp>

namespace SaiKoLily
{
    static IDiceContextProvider* s_diceContextProvider = nullptr;
    void SetDiceContextProvider(IDiceContextProvider* provider)
    {
        s_diceContextProvider = provider;
    }

    IDiceContextProvider* GetDiceContextProvider()
    {
        return s_diceContextProvider;
    }
}



namespace Sessions
{
    static Sessions_LogCallback cmds_logCallback = nullptr;
    void SetLogCallback(Sessions_LogCallback callback)
    {
        cmds_logCallback = callback; 
    }
}

namespace Sessions::Session
{
    //Session
    class SessionImpl : public SessionTemp::Session
    {
    public:
        explicit SessionImpl(ID _id,
            std::optional<uint64> _creatorID, 
            std::optional<uint64> _channelID, 
            std::optional<uint64> _serverID,
            SessionsType _type,
            std::string_view _creatorName = "Unknow",
            std::string_view _channelName = "Unknow",
            std::string_view _serverName = "Unknow",
            const std::string_view _title = "Unknow",
            const std::string_view _description = "NULL",
            std::unique_ptr<IDiceSystem> _diceSystem = nullptr 
        ) : 
        sessionID(_id),
        creatorID(_creatorID.value_or(0)),
        channelID(_channelID.value_or(0)),
        serverID(_serverID.value_or(0)),
        creatorName(_creatorName),
        channelName(_channelName),
        serverName(_serverName),
        type(_type),
        description(_description),
        dice_system(std::move(_diceSystem))
        {
            if(!dice_system)
            {
                cmds_logCallback("SessionImpl: DiceSystem 未提供，会话需要一个DiceSystem！请作废此Session，并立即释放内存！", Plugin_Logs::logLevel::err, true);
            }

            time_created = std::chrono::utc_clock::now();
        }
        
        
        ~SessionImpl() = default;

        //禁止拷贝和赋值
        SessionImpl(const SessionImpl&) = delete;
        SessionImpl operator=(const SessionImpl&) = delete;

        const ID GetID() const override
        {
            return sessionID;
        }
        const uint64 GetCreateID() const override
        {
            return creatorID;
        }
        const uint64 GetChannelID() const override
        {
            return channelID;
        }
        const uint64 GetServerID() const override
        {
            return serverID;
        }
        const std::string_view GetCreateName() const override
        {
            return creatorName;
        }
        const std::string_view GetChannelName() const override
        {
            return channelName;
        }
        const std::string_view GetServerName() const override
        {
            return serverName;
        }
        const SessionsType GetSessionType() const override
        {
            return type;
        }
        const std::string_view GetTitle() const override
        {
            return title;
        }
        const std::string_view GetDescription() const override
        {
            return description;
        }

        const std::vector<Command_Core::User> GetUsers() const override
        {
            return users;
        }

        const std::vector<Command_Core::Admin> GetAdmins() const override
        {
            return admins;
        }

        const std::chrono::utc_clock::time_point GetCreateTime() const override
        {
            return time_created;
        }

        const std::chrono::utc_clock::time_point GetAliveTime() const override
        {
            const auto end = std::chrono::utc_clock::now();
            const std::chrono::duration<double> diff = end - time_created;
            return std::chrono::utc_clock::time_point(std::chrono::duration_cast<std::chrono::utc_clock::duration>(diff));
        }

        const size_t GetUserCount() const override
        {
            return users.size();
        }

        const size_t GetAdminCount() const override
        {
            return admins.size();
        }




        void SetID(ID sessionID) override
        {
            this->sessionID = sessionID;
        }
        void SetCreatorID(ID creatorID) override
        {
            this->creatorID = creatorID;
        }
        void SetChannelID(ID channelID) override
        {
            this->channelID = channelID;
        }
        void SetServerID(ID serverID) override
        {
            this->serverID = serverID;
        }
        void SetCreatorName(std::string_view creatorName) override
        {
            this->creatorName = creatorName.data();
        }
        void SetChannelName(std::string_view channelName) override
        {
            this->channelName = channelName.data();
        }
        void SetServerName(std::string_view serverName) override
        {
            this->serverName = serverName.data();
        }
        void SetSessionType(SessionsType type) override
        {
            this->type = type;
        }
        void SetTitle(std::string_view title) override
        {
            this->title = title.data();
        }
        void SetDescription(std::string_view description) override
        {
            this->description = description.data();
        }
        virtual void AddUsers(std::vector<Command_Core::User> users) override
        {
            if(users.empty())
            {
                return;
            }

            for(auto& user : users)
            {
                this->users.push_back(user);
            }
        }
        virtual void AddAdmins(std::vector<Command_Core::Admin> admins) override
        {
            for (auto& admin : admins)
            {
                this->admins.push_back(admin);
            }
        }

        virtual void RemoveUsers(std::vector<Command_Core::User> users) override
        {
            for(auto& user : users)
            {
                this->users.erase(std::remove_if(this->users.begin(), this->users.end(), [&user](const Command_Core::User& u) { return u.userID == user.userID; }), this->users.end());
            }
        }
        virtual void RemoveAdmins(std::vector<Command_Core::Admin> admins) override
        {
            for (auto& admin : admins)
            {
                this->admins.erase(std::remove_if(this->admins.begin(), this->admins.end(), [&admin](const Command_Core::Admin& a) { return a.userID == admin.userID; }), this->admins.end());
            }
        }

        //获取骰子系统
        std::unique_ptr<IDiceSystem>& GetDiceSystem() override
        {
            return dice_system;
        }


        //功能函数
        bool DistinctUsers(Command_Core::User user) override
        {
            for(auto& user_item : users)
            {
                if(user.userID == user_item.userID)
                {
                    return false;
                }
            }
            return true;
        }


    private:
        ID sessionID;
        uint64 creatorID;
        uint64 channelID;
        uint64 serverID;
        std::string creatorName;
        std::string channelName;
        std::string serverName;
        SessionsType type;
        std::string title;
        std::string description;
        std::unique_ptr<IDiceSystem> dice_system;
        std::vector<Command_Core::User> users;
        std::vector<Command_Core::Admin> admins;
        std::chrono::utc_clock::time_point time_created;
    };

}

namespace Sessions::SessionManager
{
    using SessionsPtr = std::unique_ptr<SessionTemp::Session>;

    //用户管理类。
    // 用户管理实现类
    class UserManagerImpl : public IUserManager
    {
    public:

        //TODO:创建映射优化性能至O（1）
        std::string GetUserName(uint64_t ServerConnectionHandler, ID userID) override
        {
            std::shared_lock<std::shared_mutex> lock(mutex);

            for(auto& user: clientCache.GetClientCache()[ServerConnectionHandler])
            {
                if(user.second.clientID == userID)
                {
                    cmds_logCallback(std::format("获取用户名成功:{0}", user.second.nickName), Plugin_Logs::logLevel::info, false);
                    return user.second.nickName;
                }
                
            }
            cmds_logCallback(std::format("获取用户名失败:{0}", userID), Plugin_Logs::logLevel::warn, false);
            return "";
        }
        std::unordered_map<ID, std::string> SearchUserID(uint64_t ServerConnectionHandler, std::string_view userName) override
        {
            std::shared_lock<std::shared_mutex> lock(mutex);
            std::unordered_map<ID, std::string> result;

            for(auto& user : clientCache.GetClientCache()[ServerConnectionHandler])
            {
                if(user.second.nickName.find(userName) != std::string::npos)
                {
                    result.try_emplace(user.second.clientID, user.second.nickName);
                }
            }
            cmds_logCallback(std::format("搜索用户名成功，数量:{0}", result.size()), Plugin_Logs::logLevel::info, false);
            return result;
        }
        std::unordered_map<ID, std::string> GetAllUsers() override
        {
            std::shared_lock<std::shared_mutex> lock(mutex);
            std::unordered_map<ID, std::string> result;

            for(auto& user : clientCache.GetClientCache())
            {
                for(auto& user : user.second)
                {
                    result.try_emplace(user.second.clientID, user.second.nickName);
                }
            }
            cmds_logCallback(std::format("获取所有用户成功，数量:{0}", result.size()), Plugin_Logs::logLevel::info, false);
            return result;
        }
    private:
        std::shared_mutex mutex;
        //获取用户信息映射
        Command_Core::IClientInfoStorage& clientCache = Command_Core::GetClientInfoStorage();
    };

    //SessionManager实现类
    class SessionManagerImpl : public Sessions::SessionManagerTemp::SessionManager
    {
    public:
        ~SessionManagerImpl() = default;

        static SessionManagerImpl& GetInstance()
        {
            static SessionManagerImpl instance;
            return instance;
        }

        //禁止拷贝
        SessionManagerImpl(const SessionManagerImpl&) = delete;
        SessionManagerImpl& operator=(const SessionManagerImpl&) = delete;

        //TODO：会话实现
        //创建会话虚函数等待实现
        //结束会话虚函数等待实现
        ID CreateSession(const Command_Core::ICommandContext& context) override
        {
            std::lock_guard<std::mutex> session_manager_lock(mutex);

            ID session_id = next_session_id++;

            context.Log(std::format("SessionManagerImpl:正在创建会话：{}，创建人：{}", session_id, context.GetCallingUserName()), Plugin_Logs::logLevel::info, false);

            //对SessionType进行转换用以适配Session创建
            auto session_type = static_cast<Sessions::SessionsType>(context.GetMessageTarget());
            
            //TODO: 提供DiceSystem工厂的其他创建函数以创建不同类型的DiceSystem
            auto dice_system = SaiKoLily::DiceSystem::GetDiceSystemFactory()->CreateDiceSystem();


            SessionsPtr session = std::make_unique<Session::SessionImpl>(
                session_id,
                context.GetCallingUserID(),
                context.GetChannelID(),
                context.GetServerID(),
                session_type,
                context.GetCallingUserName(),
                context.GetChannelName(),
                context.GetServerName(),
                "NULL",
                "NULL",
                std::move(dice_system)
            );

            //释放Dice_System指针所有权。
            dice_system.release();

            sessions.try_emplace(session_id, std::move(session));

            return session_id;
        };

        void EndSession(ID sessionID) override
        {
            std::lock_guard<std::mutex> session_manager_lock(mutex);
            //删除会话
            sessions.erase(sessionID);
        };

        bool InitSession(ID sessionID, const Command_Core::ICommandContext& context, std::string_view title, std::string_view description) override
        {
            std::lock_guard<std::mutex> session_manager_lock(mutex);
            
            if(sessions.contains(sessionID))
            {
                if(sessions.at(sessionID)->GetSessionType() == Sessions::SessionsType::Channel)
                {
                    sessions.at(sessionID)->SetChannelID(context.GetChannelID());
                    sessions.at(sessionID)->SetChannelName(context.GetChannelName());
                    sessions.at(sessionID)->SetServerID(context.GetServerID());
                    sessions.at(sessionID)->SetServerName(context.GetServerName());
                    sessions.at(sessionID)->SetCreatorName(context.GetCallingUserName());
                    sessions.at(sessionID)->SetTitle(title);
                    sessions.at(sessionID)->SetDescription(description);
                }
                else if(sessions.at(sessionID)->GetSessionType() == Sessions::SessionsType::Private)
                {
                    sessions.at(sessionID)->SetCreatorID(context.GetCallingUserID());
                    sessions.at(sessionID)->SetCreatorName(context.GetCallingUserName());
                    sessions.at(sessionID)->SetTitle(title);
                    sessions.at(sessionID)->SetDescription(description);
                }
                else
                {
                    sessions.at(sessionID)->SetCreatorID(context.GetCallingUserID());
                    sessions.at(sessionID)->SetCreatorName(context.GetCallingUserName());
                    sessions.at(sessionID)->SetServerID(context.GetServerID());
                    sessions.at(sessionID)->SetServerName(context.GetServerName());
                    sessions.at(sessionID)->SetTitle(title);
                    sessions.at(sessionID)->SetDescription(description);
                }
                return true;
            }
            else
            {
                if(context.GetMessageTarget() == Command_Core::MessageTarget::CurrentChannel)
                {
                    cmds_logCallback(std::format("SessionManagerImpl:会话{}初始化失败，会话不存在！", sessionID), Plugin_Logs::logLevel::err, false);
                    context.SendResult(std::format("会话初始化失败，会话不存在！"), Command_Core::MessageTarget::CurrentChannel);
                }
                else if(context.GetMessageTarget() == Command_Core::MessageTarget::Server)
                {
                    cmds_logCallback(std::format("SessionManagerImpl:会话{}初始化失败，会话不存在！", sessionID), Plugin_Logs::logLevel::err, false);
                    context.SendResult(std::format("会话初始化失败，会话不存在！"), Command_Core::MessageTarget::Server);
                }
                else
                {
                    cmds_logCallback(std::format("SessionManagerImpl:会话{}初始化失败，会话不存在！", sessionID), Plugin_Logs::logLevel::err, false);
                    context.SendResult(std::format("会话初始化失败，会话不存在！"), Command_Core::MessageTarget::PrivateMessage);
                }
            }
            return false;
        }
        void SaveSession(ID sessionID) override 
        {

        }
        bool LoadSession(ID sessionID) override 
        {
            return true;
        }
        bool AddUser(ID sessionID, std::vector<Command_Core::User> users, const Command_Core::ICommandContext& context) override 
        {
            std::lock_guard session_manager_lock(mutex);
            if(sessions.contains(sessionID))
            {
                if(sessionID == 0)
                {
                    context.SendResult("添加用户失败，Session不存在！", Command_Core::MessageTarget::CurrentChannel);
                    return false;
                }

                context.Log(std::format("发现SessionID：{}!", sessionID), Plugin_Logs::logLevel::info, false);

                if(users.empty())
                {
                    context.SendResult("添加用户失败，用户列表为空！", Command_Core::MessageTarget::CurrentChannel);
                    return false;
                }
                users.erase(std::remove_if(users.begin(), users.end(), [](auto& user) { return user.uuid == ""; }), users.end());
                std::erase_if(users, [&](const auto& user) {
                    return !(sessions.at(sessionID)->DistinctUsers(user));
                });

                if(users.empty())
                {
                    context.SendResult(std::format("添加用户失败，所添加的所有用户均已存在在Session：{}({})中", sessionID, sessions.at(sessionID)->GetTitle()), Command_Core::MessageTarget::CurrentChannel);
                    return false;
                }

                sessions.at(sessionID)->AddUsers(users);

                std::string ret = std::format("SessionID：{} SessionTitle：{} SessionDescription：{} 添加用户成功！", sessionID, sessions.at(sessionID)->GetTitle(), sessions.at(sessionID)->GetDescription());

                for(auto& user : users)
                {
                    ret += std::format("用户：{}\t", user.name);
                }
                context.SendResult("添加用户："+ret, Command_Core::MessageTarget::CurrentChannel);
                context.Log("添加用户："+ret, Plugin_Logs::logLevel::info, false);
            }
            else
            {
                context.SendResult("添加用户失败，Session不存在！", Command_Core::MessageTarget::CurrentChannel);
                return false;
            }
            return true;
        }
        bool RemoveUser(ID sessionID, std::vector<Command_Core::User> users, const Command_Core::ICommandContext& context) override 
        {
            std::lock_guard session_manager_lock(mutex);
            if(sessions.contains(sessionID))
            {
                if(sessionID == 0)
                {
                    context.SendResult("删除用户失败，Session不存在！", Command_Core::MessageTarget::CurrentChannel);
                    return false;
                }

                context.Log(std::format("发现SessionID：{}!", sessionID), Plugin_Logs::logLevel::info, false);

                if(users.empty())
                {
                    context.SendResult("添加用户失败，用户列表为空！", Command_Core::MessageTarget::CurrentChannel);
                    return false;
                }

                //无法删除空用
                users.erase(std::remove_if(users.begin(), users.end(), [](auto& user) { return user.uuid == ""; }), users.end());
                //删除待删除列表中的重复用户
                std::erase_if(users, [&](const auto& user) {
                    return sessions.at(sessionID)->DistinctUsers(user);
                });

                for(auto& user : users)
                {
                    context.SendResult(std::format("发现用户：{}!", user.name), Command_Core::MessageTarget::CurrentChannel);
                }

                if(users.empty())
                {
                    context.SendResult(std::format("删除用户失败，所添加的所有用户均未存在在Session：{}({})中", sessionID, sessions.at(sessionID)->GetTitle()), Command_Core::MessageTarget::CurrentChannel);
                    return false;
                }

                sessions.at(sessionID)->RemoveUsers(users);
               
                std::string ret = std::format("SessionID：{} SessionTitle：{} SessionDescription：{} 删除用户成功！", sessionID, sessions.at(sessionID)->GetTitle(), sessions.at(sessionID)->GetDescription());

                for(auto& user : users)
                {
                    ret += std::format("用户：{}\t", user.name);
                }
                context.SendResult("删除用户："+ret, Command_Core::MessageTarget::CurrentChannel);
                context.Log("删除用户："+ret, Plugin_Logs::logLevel::info, false);
            }
            else
            {
                context.SendResult("删除用户失败，Session不存在！", Command_Core::MessageTarget::CurrentChannel);
                return false;
            }
            return true;
        }
        bool SetAdmin(ID sessionID, std::vector<Command_Core::User> user, const Command_Core::ICommandContext& context) override 
        {
            return true;
        }
        bool RemoveAdmin(ID sessionID, std::vector<Command_Core::User> user, const Command_Core::ICommandContext& context) override 
        {
            return true;
        }
        std::vector<Command_Core::User> GetUsers(ID sessionID, const Command_Core::ICommandContext& context)  override 
        {
            std::lock_guard session_manager_lock(mutex);

            if(sessions.contains(sessionID))
            {
                auto users = sessions.at(sessionID)->GetUsers();

                if(!users.empty())
                {
                    auto ret = std::format("SessionsID：{}存在以下用户：", sessionID);
                    for(auto& user : users)
                    {
                        ret += user.name + "\t";
                    }

                    context.SendResult(ret, Command_Core::MessageTarget::CurrentChannel);
                }

                return users;
            }

            return {};
        }
        std::vector<Command_Core::Admin> GetAdmins(ID sessionID, const Command_Core::ICommandContext& context) override 
        {
            return sessions.at(sessionID)->GetAdmins();
        }
        std::vector<ID> GetSessionLists () override 
        {
            std::vector<ID> ret;
            for(auto& session : sessions)
            {
                ret.push_back(session.first);
            }
            return ret;
        }
        void PauseSession(ID sessionID) override 
        {
            cmds_logCallback(std::format("SessionManagerImpl:暂停会话：{}", sessionID), Plugin_Logs::logLevel::info, false);
        }
        void ResumeSession(ID sessionID) override 
        {
            cmds_logCallback(std::format("SessionManagerImpl:恢复会话：{}", sessionID), Plugin_Logs::logLevel::info, false);
        }
    

        //功能函数
        const std::unordered_map<std::string, ID>& GetSelectionOfSession() const override
        {
            return each_user_selected_sessions;
        }

        void UserSelectSession(const SessionTemp::Session& session, const Command_Core::User& user) override
        {
            if(each_user_selected_sessions.contains(user.uuid))
            {
                cmds_logCallback(std::format("用户:{}({})选择了Session:{}({})，但是该用户已选择过Session:{}({}),于是将其更新为当前选择的最新Session！", user.name, user.userID, session.GetID(), session.GetTitle(), each_user_selected_sessions.at(user.uuid), sessions.at(each_user_selected_sessions.at(user.uuid))->GetTitle()), Plugin_Logs::logLevel::warn, false);
                each_user_selected_sessions.at(user.uuid) = session.GetID();
                return;
            }
            else
            {
                cmds_logCallback(std::format("用户:{}({})选择了Session:{}({})", user.name, user.userID, session.GetID(), session.GetTitle()), Plugin_Logs::logLevel::info, false);
            }
            each_user_selected_sessions.try_emplace(user.uuid, session.GetID());
        }

        void UserUnselectSession(const SessionTemp::Session& session, const Command_Core::User& user) override
        {
            if(!each_user_selected_sessions.contains(user.uuid))
            {
                cmds_logCallback(std::format("用户:{}({})取消选择了Session:{}({})，但是该用户未选择过Session", user.name, user.userID, session.GetID(), session.GetTitle()), Plugin_Logs::logLevel::warn, false);
                return;
            }
            else
            {
                cmds_logCallback(std::format("用户:{}({})取消选择了Session:{}({})", user.name, user.userID, session.GetID(), session.GetTitle()), Plugin_Logs::logLevel::info, false);
            }
            each_user_selected_sessions.erase(user.uuid);
        }


        //传递消息
        SaiKoLily::DiceSystem::IDiceSystem* GetDiceSystem(const Command_Core::ICommandContext& context) 
        {
            std::lock_guard<std::mutex> lock(mutex);
            
            //获取当前用户  
            ID targetSession_id = 0;
            ID userID = context.GetCallingUserID();
            std::string userUUID(context.GetUserUUID(context.GetCallingUserID()));

            bool isUserSelectedSession = each_user_selected_sessions.contains(userUUID);

            //第一优先级
            if(isUserSelectedSession)
            {
                targetSession_id = each_user_selected_sessions.at(userUUID);
                cmds_logCallback(std::format("用户:{}({})选择了Session:{}({})，将使用该Session的骰子系统！", context.GetCallingUserName(), context.GetCallingUserID(), targetSession_id, sessions.at(targetSession_id)->GetTitle()), Plugin_Logs::logLevel::info, false);
            }
            else if(!isUserSelectedSession)   //第二优先级，如果当前用户绑定Session，则使用绑定的Session
            {
                //获取用户加入的Session
                for(const auto& session : GetSessionsList())
                {
                    for(const auto& user : session.get().GetUsers())
                    {
                        if(user.userID == userID)
                        {
                            targetSession_id = session.get().GetID();
                            cmds_logCallback(std::format("用户:{}({})在Session:{}({})中，将使用该Session的骰子系统！", context.GetCallingUserName(), context.GetCallingUserID(), targetSession_id, session.get().GetTitle()), Plugin_Logs::logLevel::info, false);
                            break;
                        }
                    }
                }
            }
            else   //第三优先级，如果当前频道绑定Session，则使用绑定的Session
            {
                Sessions::SessionFetchMethod method = Sessions::SessionFetchMethod::Index;
                if(SessionManagerImpl::GetSession(method, std::to_string(context.GetChannelID())).has_value())
                {
                    cmds_logCallback(std::format("用户:{}({})未选择Session，将尝试使用频道绑定的Session！", context.GetCallingUserName(), context.GetCallingUserID()), Plugin_Logs::logLevel::info, false);
                    targetSession_id = SessionManagerImpl::GetSession(method, std::to_string(context.GetChannelID())).value()->GetID();
                }
                else
                {
                    cmds_logCallback(std::format("用户:{}({})未选择Session，且当前频道未绑定Session，无法获取骰子系统！", context.GetCallingUserName(), context.GetCallingUserID()), Plugin_Logs::logLevel::warn, false);
                    return nullptr;
                }
            }
            
            //TODO:SessionManager绑定Session处理

            //尝试获取Session并返回DiceSystem
            if(targetSession_id != 0 && sessions.contains(targetSession_id))
            {
                auto& Session = sessions.at(targetSession_id);
                if(Session->GetDiceSystem())
                {
                    return Session->GetDiceSystem().get();
                }
            }

            return nullptr;
        }
    public:

        //by Creator, Server, Channel
        std::vector<std::reference_wrapper<SessionTemp::Session>> GetSessionsList(Sessions::SessionListFilter& method, std::string_view arg) override
        {
            if(sessions.empty())
            {
                cmds_logCallback(std::format("目前会话集为空！"), Plugin_Logs::logLevel::info, false);
            }

            std::vector<std::reference_wrapper<SessionTemp::Session>> ret;
            switch(method)
            {
            case Sessions::SessionListFilter::Creator:
                { 
                    if(arg.empty())
                    {
                        cmds_logCallback(std::format("查询的会话不存在！"), Plugin_Logs::logLevel::warn, false);
                        break;
                    }
                    
                    auto iter = sessions.begin();

                    while (true)
                    {
                        if(iter == sessions.end())
                        {
                            cmds_logCallback(std::format("查询完毕！"), Plugin_Logs::logLevel::info, false);
                            break;
                        }

                        if(iter->second->GetCreateName() == arg)
                        {
                            ret.push_back(*iter->second);
                        }
                        iter++;
                    }
                    
                    break;
                }
            case Sessions::SessionListFilter::Channel:
                {
                    if(arg.empty())
                    {
                        cmds_logCallback(std::format("查询的会话不存在！"), Plugin_Logs::logLevel::warn, false);
                        break;
                    }

                    auto iter = sessions.begin();

                    while (true)
                    {
                        if(iter == sessions.end())
                        {
                            cmds_logCallback(std::format("查询完毕！"), Plugin_Logs::logLevel::info, false);
                            break;
                        }

                        if(iter->second->GetChannelName() == arg)
                        {
                            ret.push_back(*iter->second);
                        }
                        iter++;
                    }
                    
                    break;
                }
            case Sessions::SessionListFilter::Server:
                {
                    if(arg.empty())
                    {
                        cmds_logCallback(std::format("查询的会话不存在！"), Plugin_Logs::logLevel::warn, false);
                        break;
                    }

                    auto iter = sessions.begin();

                    while (true)
                    {
                        if(iter == sessions.end())
                        {
                            cmds_logCallback(std::format("查询完毕！"), Plugin_Logs::logLevel::info, false);
                            break;
                        }

                        if(iter->second->GetServerName() == arg)
                        {
                            ret.push_back(*iter->second);
                        }
                        iter++;
                    }
                    
                    break;
                }

            }

            return ret;
        }

        // by All
        std::vector<std::reference_wrapper<SessionTemp::Session>> GetSessionsList() override
        {
            if(sessions.empty())
            {
                cmds_logCallback(std::format("目前会话集为空！"), Plugin_Logs::logLevel::info, false);
                return std::vector<std::reference_wrapper<SessionTemp::Session>>();
            }
            std::vector<std::reference_wrapper<SessionTemp::Session>> ret;
            
            auto iter = sessions.begin();
            while(true)
            {
                if(iter == sessions.end())
                {
                    cmds_logCallback(std::format("查询完毕！"), Plugin_Logs::logLevel::info, false);
                }
                ret.push_back(*iter->second);
                iter++;
            }

            return ret;
        }

        // // by Title, Creator, Index
        std::optional<SessionTemp::Session*> GetSession(Sessions::SessionFetchMethod& method, std::string_view arg) override
        {
            if(sessions.empty())
            {
                cmds_logCallback(std::format("目前会话集为空！"), Plugin_Logs::logLevel::info, false);
                return std::nullopt;
            }

            if(arg.empty())
            {
                cmds_logCallback(std::format("输入的参数为空！"), Plugin_Logs::logLevel::info, false);
                return std::nullopt;
            }

            auto iter = sessions.begin();
            
            switch(method)
            {
            case Sessions::SessionFetchMethod::Title:
                {
                    cmds_logCallback(std::format("正在通过标题查询到会话！"), Plugin_Logs::logLevel::info, false);
                    
                    while (iter != sessions.end())
                    {
                        if(iter->second->GetTitle() == arg)
                        {
                            cmds_logCallback(std::format("已查询到会话！"), Plugin_Logs::logLevel::info, false);
                            return iter->second.get();
                        }
                        iter++;
                    }

                    cmds_logCallback(std::format("未通过标题：\"{}\"查询到会话！", arg), Plugin_Logs::logLevel::warn, true);
                    break;
                }
            case Sessions::SessionFetchMethod::Creator:
                {
                    cmds_logCallback(std::format("正在通过创建者查询到会话！"), Plugin_Logs::logLevel::info, false);

                    while (iter != sessions.end())
                    {
                        if(iter->second->GetCreateName() == arg)
                        {
                            cmds_logCallback(std::format("已查询到会话！"), Plugin_Logs::logLevel::info, false);
                            return iter->second.get();
                        }
                        iter++;
                    }

                    cmds_logCallback(std::format("未通过用户名：\"{}\"查询到会话！", arg), Plugin_Logs::logLevel::warn, true);
                    break;
                }
            case Sessions::SessionFetchMethod::Index:
                {
                    cmds_logCallback(std::format("正在通过会话ID查询到会话！"), Plugin_Logs::logLevel::info, false);

                    auto Temp_String_to_ID = std::stoll(arg.data());
                    while (iter != sessions.end())
                    {
                        if(iter->first == Temp_String_to_ID)
                        {
                            cmds_logCallback(std::format("已查询到会话！"), Plugin_Logs::logLevel::info, false);
                            return iter->second.get();
                        }
                        iter++;
                    }

                    cmds_logCallback(std::format("未通过Session ID：\"{}\"查询到会话！", arg), Plugin_Logs::logLevel::warn, true);
                    break;
                }
            default:
                {
                    return std::nullopt;
                }
            }
            
            return std::nullopt;
        }

    private:
        SessionManagerImpl() {
            SaiKoLily::SetDiceContextProvider(this);
        };

    private:
        mutable std::mutex mutex;
        std::unordered_map<ID, SessionsPtr> sessions;
        //                  UUID        SessionID
        std::unordered_map<std::string, ID> each_user_selected_sessions;
        ID next_session_id = 1;
    };
}

namespace Sessions::SessionsCommand
{
    class SessionsCommandImpl : public Sessions::SessionsCommandTemp::SessionsCommand
    {
    public:
        SessionsCommandImpl() 
        {
            sessions_manager = &(SessionManager::SessionManagerImpl::GetInstance());
        }
        ~SessionsCommandImpl() = default;

        //禁止拷贝
        SessionsCommandImpl(const SessionsCommandImpl&) = delete;
        SessionsCommandImpl& operator=(const SessionsCommandImpl&) = delete;

        void SessionsCommand::Execute(Command_Core::ICommandContext& context) override
        {

            if(context.GetParamCount() <= 0)
            {
                cmds_logCallback(std::format("Sessions: 参数不足！"), Plugin_Logs::logLevel::warn, false);
                context.SendResult("Sessions: 参数不足！", Command_Core::MessageTarget::CurrentChannel);
                context.SendResult(std::format("用法： [!, .][session, sessions, game, Game, Session, Sessions, ss, SS] [会话命令]。如： .ss [会话命令]等。"), context.GetMessageTarget());
                context.Log(std::format("用法： [!, .][session, sessions, game, Game, Session, Sessions, ss, SS] [会话命令]。如： .ss [会话命令]等。"), Plugin_Logs::logLevel::warn, false);
                return;
            }

            /*
                explicit SessionImpl(ID _id, 
                    uint64 _creatorID, 
                    uint64 _channelID, 
                    uint64 _serverID,
                    std::string_view _creatorName,
                    std::string_view _channelName,
                    std::string_view _serverName,
                    SessionsType _type,
                    const std::string_view _title = "Unknow",
                    const std::string_view _description = "NULL"
                )
            */

            //获取第一个参数（检测第一个参数是否为命令表达式）
            auto exprOpt = context.GetParam(0);
            if(!exprOpt)
            {
                context.SendResult(std::format("错误！缺少会话命令！"), context.GetMessageTarget());
                context.Log(std::format("错误！缺少会话命令！"), Plugin_Logs::logLevel::err, false);
                return;
            }

            //std::string_view expr = *exprOpt;

            size_t expr_count = context.GetParamCount();
            std::vector<std::string_view> exprs;
            for(auto i = 0; i < expr_count; i++)
            {
                exprs.push_back(context.GetParam(i).value());
            }

            try
            {
                //解析命令
                auto terms = Sessions::SessionsCommand::SessionsCommandImpl::ParserExpression(exprs
                    #ifdef _Session_DEBUG
                    , context
                    #endif
                );
                //执行命令
                auto res = Sessions::SessionsCommand::SessionsCommandImpl::ExecuteExpression(terms, sessions_manager, context);

                if (!res.empty())
                {
                    context.SendResult(res);
                    cmds_logCallback(std::format("Sessions::Execute: {0}", res), Plugin_Logs::logLevel::info, false);
                }
                
            }
            catch(const std::exception& e)
            {
                cmds_logCallback(std::format("Sessions::Execute: 错误：{0}", e.what()), Plugin_Logs::logLevel::err, false);
            }

            return;
        }

        Sessions::SessionManagerTemp::SessionManager* GetSessionsManager() override
        {
            return sessions_manager;
        }
        
    private:
        Sessions::SessionManagerTemp::SessionManager* sessions_manager;
    };
}


namespace Sessions
{
    void RegisterSessionsCommand(Command_Core::ICommandRegistry &registry)
    {
        registry.RegisterHandler(std::make_unique<SessionsCommand::SessionsCommandImpl>());
    }
}


