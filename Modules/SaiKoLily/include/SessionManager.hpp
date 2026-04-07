#pragma once 

#include <memory>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>
#include <loggings.hpp>
#include <shared_mutex>
#include <CommandCore.hpp>
#include <GwongDongFileSystem.hpp>
#include <SaiKoLilyInterface.hpp>

using int64_t = long long;
using uint64 = unsigned long long;


#define _Session_DEBUG

namespace SaiKoLily {
    namespace DiceSystem {
        class IDiceSystem;
    }
}
using IDiceSystem = SaiKoLily::DiceSystem::IDiceSystem;

namespace Sessions
{
    using Sessions_LogCallback = std::function<void(const std::string&, Plugin_Logs::logLevel, bool nowFlush)>;
    void SetLogCallback(Sessions_LogCallback callback);


    extern enum class SessionFetchMethod
    {
        Title = 1,
        Creator,
        Index
    } GetSessionBy;

    extern enum class SessionListFilter
    {
        Creator = 1,
        Server,
        Channel,
        Auto
    } GetSessionListBy;

    enum class SessionsType
    {
        Private = 1,
        Channel = 2,
        Server = 3
    };

    //用户管理接口类
    class IUserManager
    {
    public:
        virtual ~IUserManager() = default;
        //通过ID获取用户名称
        virtual std::string GetUserName(uint64_t ServerConnectionHandler,ID userID) = 0;
        //通过用户名查询ID
        virtual std::unordered_map<ID, std::string> SearchUserID(uint64_t ServerConnectionHandler, std::string_view userName) = 0;
        //获取所有用户以及对应ID
        virtual std::unordered_map<ID, std::string> GetAllUsers() = 0;
    };

}

namespace Sessions::SessionTemp
{
    //Session
    class Session
    {
    public:
        Session() = default;
        
        ~Session() = default;

        //禁止拷贝和赋值
        Session(const Session&) = delete;
        Session operator=(const Session&) = delete;

        virtual const ID GetID() const = 0;
        virtual const uint64 GetCreateID() const = 0;
        virtual const uint64 GetChannelID() const = 0;
        virtual const uint64 GetServerID() const = 0;
        virtual const std::string_view GetCreateName() const = 0;
        virtual const std::string_view GetChannelName() const = 0;
        virtual const std::string_view GetServerName() const = 0;
        virtual const SessionsType GetSessionType() const = 0;
        virtual const std::string_view GetTitle() const = 0;
        virtual const std::string_view GetDescription() const = 0;
        virtual const std::vector<Command_Core::User> GetUsers() const = 0;
        virtual const std::vector<Command_Core::Admin> GetAdmins() const = 0;
        virtual const std::chrono::utc_clock::time_point GetCreateTime() const = 0;
        virtual const std::chrono::utc_clock::time_point GetAliveTime() const = 0;
        virtual const size_t GetUserCount() const = 0;
        virtual const size_t GetAdminCount() const = 0; 


        virtual void SetID(ID sessionID) = 0;
        virtual void SetCreatorID(ID creatorID) = 0;
        virtual void SetChannelID(ID channelID) = 0;
        virtual void SetServerID(ID serverID) = 0;
        virtual void SetCreatorName(std::string_view creatorName) = 0;
        virtual void SetChannelName(std::string_view channelName) = 0;
        virtual void SetServerName(std::string_view serverName) = 0;
        virtual void SetSessionType(SessionsType sessionType) = 0;
        virtual void SetTitle(std::string_view title) = 0;
        virtual void SetDescription(std::string_view description) = 0;
        virtual void AddUsers(std::vector<Command_Core::User> users) = 0;
        virtual void AddAdmins(std::vector<Command_Core::Admin> admins) = 0;
        virtual void RemoveUsers(std::vector<Command_Core::User> users) = 0;
        virtual void RemoveAdmins(std::vector<Command_Core::Admin> admins) = 0;


        virtual bool DistinctUsers(Command_Core::User user) = 0;

        //获取骰子系统
        virtual std::unique_ptr<IDiceSystem>& GetDiceSystem() = 0;
        
    };


}


namespace Sessions::SessionManagerTemp
{
    using SessionsPtr = std::unique_ptr<SessionTemp::Session>;

    //SessionManager
    //TODO:修改SessionManager为单例模式
    class SessionManager : public SaiKoLily::IDiceContextProvider
    {
    public:
        virtual ~SessionManager() = default;

        SessionManager() = default;

        //禁止拷贝
        SessionManager(const SessionManager&) = delete;
        SessionManager& operator=(const SessionManager&) = delete;

        virtual ID CreateSession(const Command_Core::ICommandContext& context) = 0;
        virtual void EndSession(ID sessionID) = 0;
        virtual bool InitSession(ID sessionID, const Command_Core::ICommandContext& context, std::string_view title, std::string_view description) = 0;
        virtual void SaveSession(ID sessionID) = 0;
        virtual bool LoadSession(ID sessionID) = 0;
        virtual bool AddUser(ID sessionID, std::vector<Command_Core::User> user, const Command_Core::ICommandContext& context) = 0;
        virtual bool RemoveUser(ID sessionID, std::vector<Command_Core::User> user, const Command_Core::ICommandContext& context) = 0;
        virtual bool SetAdmin(ID sessionID, std::vector<Command_Core::User> user, const Command_Core::ICommandContext& context) = 0;
        virtual bool RemoveAdmin(ID sessionID, std::vector<Command_Core::User> user, const Command_Core::ICommandContext& context) = 0;
        virtual std::vector<Command_Core::User> GetUsers(ID sessionID, const Command_Core::ICommandContext& context)  = 0;
        virtual std::vector<Command_Core::Admin> GetAdmins(ID sessionID, const Command_Core::ICommandContext& context) = 0;
        virtual std::vector<ID> GetSessionLists () = 0;
        virtual void PauseSession(ID sessionID) = 0;
        virtual void ResumeSession(ID sessionID) = 0;


        //功能函数
        virtual const std::unordered_map<std::string, ID>& GetSelectionOfSession() const = 0;
        virtual void UserSelectSession(const SessionTemp::Session& session, const Command_Core::User& user) = 0;
        virtual void UserUnselectSession(const SessionTemp::Session& session, const Command_Core::User& user) = 0;

        //传递消息
        SaiKoLily::DiceSystem::IDiceSystem* GetDiceSystem(const Command_Core::ICommandContext& context) override = 0;

    public:

        //by Creator, Server, Channel
        virtual std::vector<std::reference_wrapper<SessionTemp::Session>> GetSessionsList(Sessions::SessionListFilter& method, std::string_view arg) = 0;
        //by All
        virtual std::vector<std::reference_wrapper<SessionTemp::Session>> GetSessionsList() = 0;
        //by Title, Creator, Index
        virtual std::optional<SessionTemp::Session*> GetSession(Sessions::SessionFetchMethod& method, std::string_view arg) = 0;
    };


}


namespace Sessions::SessionsCommandTemp
{
    /*
    创建会话，结束会话，查询会话信息，查询会话列表，搜索会话，激活会话，会话休眠，用户选择会话，用户解除选择会话，用户加入，用户退出，设置管理员，移除管理员，管理员列表，用户列表，绑定会话至，消息记录日志，删除消息记录日志，修改会话，导入会话，导出会话，命令传入。
    */
    enum class TermType
    {
        Create = 1, End, Status, Info, List, Search, Activate, Rest, Select, UnSelect, UserJoin, UserQuit, AdminsSet, AdminsRemove, AdminsList, UserList, Bind, History, CleanHistory, SaveToFile, Set, Export, Import, Help, Others, NULLCommand
    };

    static std::unordered_map<std::string_view, Sessions::SessionFetchMethod> FetchMethodMap =
    {
        {"Index", Sessions::SessionFetchMethod::Index},
        {"index", Sessions::SessionFetchMethod::Index},
        {"ID", Sessions::SessionFetchMethod::Index},
        {"id", Sessions::SessionFetchMethod::Index},
        {"Title", Sessions::SessionFetchMethod::Title},
        {"title", Sessions::SessionFetchMethod::Title},
        {"t", Sessions::SessionFetchMethod::Title},
        {"Creator", Sessions::SessionFetchMethod::Creator},
        {"creator", Sessions::SessionFetchMethod::Creator},
        {"C", Sessions::SessionFetchMethod::Creator}
    };

    //唯一性的命令
    struct ExpressionTerm
    {
        using Args = std::string;
        TermType type = TermType::NULLCommand;
        std::vector<Args> args;
    };

    class SessionsCommand : public Command_Core::ICommandHandler
    {
    public:
        SessionsCommand() = default;
        
        ~SessionsCommand() = default;

        //禁止移动与拷贝构造
        SessionsCommand(SessionsCommand& other) = delete;
        SessionsCommand& operator=(SessionsCommand& other) = delete;

    public:
        //由其派生类来实现
        //virtual void Execute(Command_Core::ICommandContext& context) = 0;

        std::string_view GetName() const override
        {
            return "Sessions";
        }
        std::string_view GetDescription() const override
        {
            return "会话管理命令\"Session\"。用来管理频道会话，可以用来创建会话并在会话中记录各种信息与提取会话包含的信息。";
        }
        std::vector<std::string_view> GetAlias() const override
        {
            return {"session", "sessions", "game", "Game", "Session", "Sessions", "ss", "SS"};
        }

    public:
        virtual Sessions::SessionManagerTemp::SessionManager* GetSessionsManager() = 0;

    protected:
        static std::vector<ExpressionTerm> ParserExpression(std::vector<std::string_view> exprs
            #ifdef _Session_DEBUG
            , Command_Core::ICommandContext& context
            #endif
        )
        {
            std::vector<ExpressionTerm> Terms;
            
            context.Log(std::format("命令解析前未出错"), Plugin_Logs::logLevel::info, true);

            if(exprs.empty())
            {
                context.Log("Sessions: 命令为空！", Plugin_Logs::logLevel::warn, false);
                return Terms;
            }

            for(auto& expr : exprs)
            {
                context.SendResult(std::format("Sessions: {}", expr), Command_Core::MessageTarget::CurrentChannel);
                context.Log(std::format("Sessions: {}", expr), Plugin_Logs::logLevel::info, false);
            }

            for(auto& expr : exprs)
            {
                if(commands.contains(expr))
                {
                    Terms.emplace_back(commands[expr], std::vector<ExpressionTerm::Args>{});
                }
                else if(!commands.contains(expr) && !Terms.empty())
                {
                    context.SendResult(std::format("添加参数:{}", expr), Command_Core::MessageTarget::CurrentChannel);
                    Terms.back().args.push_back(std::string(expr));
                }
                else
                {
                    context.Log(std::format("Sessions: 命令 {} 不存在！", expr), Plugin_Logs::logLevel::warn, false);
                    context.SendResult(std::format("Sessions: 命令 {} 不存在！", expr), Command_Core::MessageTarget::CurrentChannel);
                }
            }

            if(Terms.empty())
            {
                context.Log("Sessions: 命令为空！", Plugin_Logs::logLevel::warn, false);
                context.SendResult("Sessions: 命令为空！", Command_Core::MessageTarget::CurrentChannel);
            }

            for(auto& term : Terms)
            {
                if(term.type == TermType::NULLCommand)
                {
                    context.Log(std::format("{} 命令为空！", static_cast<int>(term.type)), Plugin_Logs::logLevel::warn, false);
                    context.SendResult(std::format("Sessions: 命令 {} 为空！", static_cast<int>(term.type)), Command_Core::MessageTarget::CurrentChannel);
                }
                else
                {
                    context.Log(std::format("Sessions: 命令 {} 已被解析！", static_cast<int>(term.type)), Plugin_Logs::logLevel::info, false);
                    context.SendResult(std::format("Sessions: 命令 {} 已被解析！", static_cast<int>(term.type)), Command_Core::MessageTarget::CurrentChannel);
                }
            }

            context.Log(std::format("命令解析完毕"), Plugin_Logs::logLevel::info, true);

            return Terms;
        }
        static std::string ExecuteExpression(const std::vector<Sessions::SessionsCommandTemp::ExpressionTerm>& terms, Sessions::SessionManagerTemp::SessionManager* manager, Command_Core::ICommandContext& context)
        {
            //首先先查询并获取该用户在SessionManager中的SelectedSessionID映射。
            ID UserSelectedSession = 0;
            SessionTemp::Session* WorkingSession;
            if(manager->GetSelectionOfSession().contains(context.GetUserUUID(context.GetCallingUserID()).data()))
            {
                UserSelectedSession = manager->GetSelectionOfSession().at(context.GetUserUUID(context.GetCallingUserID()).data());

                auto method = Sessions::SessionFetchMethod::Index;

                WorkingSession = manager->GetSession(method, std::to_string(UserSelectedSession)).value_or(nullptr);
            }

            //获取工作Session的Lambda函数包装
            // std::function<std::optional<Sessions::SessionTemp::Session *>(Sessions::SessionFetchMethod& method, std::string_view arg)> GetWorkingSession = [&](Sessions::SessionFetchMethod& method, std::string_view arg) 
            // {
            //     if(UserSelectedSession == 0)
            //     {
            //         if(method == Sessions::SessionFetchMethod::Creator)
            //         {
            //             return manager->GetSession(method, context.GetCallingUserName());
            //         }
            //         else if(method == Sessions::SessionFetchMethod::Index)
            //         {
            //             return manager->GetSession(method, arg);
            //         }
            //         else if(method == Sessions::SessionFetchMethod::Title)
            //         {
            //             return manager->GetSession(method, arg);
            //         }
            //         else
            //         {
            //             //默认使用创建者
            //             return manager->GetSession(method, context.GetCallingUserName());
            //         }
            //     }
            // };

            //将所有Select移动至开头防止多次重复定义。

            auto Copy_Of_Terms = terms;
            std::stable_partition(Copy_Of_Terms.begin(), Copy_Of_Terms.end(), [](const auto& term) { return term.type == TermType::Select; });
            //最后一个Select会被最终执行。

            for(auto& term : Copy_Of_Terms)
            {
                context.Log(std::format("Sessions: 命令 {} 开始执行", static_cast<int>(term.type)), Plugin_Logs::logLevel::info, true);
                switch(term.type)
                {
                    case TermType::Create: 
                        {   
                            //检查创建会话参数是否包含Title和Description
                            if(term.args.size() < 1)
                            {
                                //需要一个外部传入的DiceSystem;
                                
                                auto ID = manager->CreateSession(context);
                                manager->InitSession(ID, context, "<Unknown Title>", "No Description.");
                            }
                            else if (term.args.size() < 2)
                            {
                                //需要一个外部传入的DiceSystem;
                                auto ID = manager->CreateSession(context);
                                manager->InitSession(ID, context, term.args[0], "No Description.");
                            }
                            else
                            {
                                auto ID = manager->CreateSession(context);
                                manager->InitSession(ID, context, term.args[0], term.args[1]);
                            }
                        }
                        break;
                    case TermType::End: 
                        break;
                    case TermType::Status:
                        break;
                    case TermType::Info:
                        {
                            context.Log(std::format("Sessions:获取会话信息"), Plugin_Logs::logLevel::info, true);

                            Sessions::SessionTemp::Session * info;

                            if(UserSelectedSession == 0)
                            {
                                auto method = Sessions::SessionFetchMethod::Creator;
                                info = manager->GetSession(method, context.GetCallingUserName()).value_or(nullptr);
                                if(info == nullptr)
                                {
                                    context.SendResult("Sessions: 没有会话！", Command_Core::MessageTarget::CurrentChannel);
                                    return "Sessions: 没有会话！";
                                }
                            }
                            else
                            {
                                info = WorkingSession;
                            }
                            

                            if(info != nullptr)
                            {
                                std::string session_type;
                                switch (info->GetSessionType())
                                {
                                case Sessions::SessionsType::Private:
                                    session_type = "Private";
                                    break;
                                case Sessions::SessionsType::Channel:
                                    session_type = "Channel";
                                    break;
                                case Sessions::SessionsType::Server:
                                    session_type = "Server";
                                    break;
                                default:
                                    break;
                                }

                                // 处理时间点到字符串的转换
                                auto timePoint = info->GetCreateTime();
                                std::time_t timeT = std::chrono::system_clock::to_time_t(
                                    std::chrono::system_clock::from_time_t(0) + 
                                    std::chrono::duration_cast<std::chrono::system_clock::duration>(
                                        timePoint.time_since_epoch()
                                    )
                                );

                                std::string createTimeStr = std::ctime(&timeT);
                                // 移除ctime添加的换行符
                                if(!createTimeStr.empty() && createTimeStr.back() == '\n') {
                                    createTimeStr.pop_back();
                                }

                                context.SendResult(std::format("Sessions: 获取会话信息成功！\n会话ID: {}\n会话标题: {}\n会话描述: {}\n会话类型: {}\n会话创建时间: {}\n会话创建者: {}\n会话创建者ID: {}", info->GetID(), info->GetTitle(), info->GetDescription(), session_type, createTimeStr, info->GetCreateName(), info->GetCreateID()), Command_Core::MessageTarget::CurrentChannel);

                                std::string user_info_string = [&]()
                                    {
                                        std::string result;
                                        for(auto&& user : info->GetUsers())
                                        {
                                            result += std::format("{}({})\t\t", user.name, user.userID);
                                        }
                                        return result;
                                    }();

                                context.SendResult(std::format("会话中人数：{}, 会话用户：\n{}\n", info->GetUserCount(), user_info_string), Command_Core::MessageTarget::CurrentChannel
                                );
                            }
                            else
                            {
                                context.SendResult(std::format("Sessions: 获取会话信息失败！"), Command_Core::MessageTarget::CurrentChannel);
                            }
                        }
                        break;
                    case TermType::List: 
                        {
                            auto list = manager->GetSessionLists();

                            std::string result;

                            auto method_id = Sessions::SessionFetchMethod::Index;

                            for (auto &&iter : list)
                            {
                                result += std::format("SessionsID: {} \t SessionsName: {}\n", iter, manager->GetSession(method_id, std::to_string(iter)).value()->GetTitle());
                            }
                            
                            context.Log(std::format("Sessions: 获取会话列表成功！\n列表：{}", result), Plugin_Logs::logLevel::info, false);
                            context.SendResult(std::format("Sessions: 获取会话列表成功！\n目前的活动列表：\n{}", result), Command_Core::MessageTarget::CurrentChannel);
                        }
                        break;
                    case TermType::Search: 
                        break;
                    case TermType::Activate: 
                        break;
                    case TermType::Rest: 
                        break;
                    case TermType::Select:
                        {
                            //通过参数获取查找方式
                            auto method = FetchMethodMap.at(term.args[0]);

                            if(term.args.size() < 2 || term.args.size() > 2)
                            {
                                context.SendResult("SessionsSelect: 参数不足！", Command_Core::MessageTarget::CurrentChannel);
                                return "SessionsSelect: 参数不足！";
                            }

                            //获取会话
                            auto session_ = manager->GetSession(method, term.args[1]);

                            if(!session_.has_value())
                            {
                                context.SendResult("SessionsSelect: 获取会话失败！", Command_Core::MessageTarget::CurrentChannel);
                                return "SessionsSelect: 获取会话失败！";
                            }
                            
                            //获取并构造一个用户
                            auto tmp = context.GetUserByName(context.GetCallingUserName());
                            Command_Core::User user(tmp.nickName, tmp.channelID, context.GetServerID(), tmp.clientID, tmp.uniqueID);

                            //添加用户UUID到被选择Session的映射
                            manager->UserSelectSession(*session_.value(), user);
                        }
                        break;
                    case TermType::UnSelect:
                        {
                            if(term.args.size() > 2)
                            {
                                context.SendResult("SessionsSelect: 参数过多！", Command_Core::MessageTarget::CurrentChannel);
                                return "SessionsSelect: 参数过多！";
                            }

                            //通过参数获取查找方式

                            if(term.args.size() < 1)
                            {
                                //获取当前用户默认选择的ID
                                auto selectedSessionID = manager->GetSelectionOfSession().at(context.GetUserUUID(context.GetCallingUserID()).data());
                                context.SendResult(std::format("SessionsSelect: 获取当前用户默认选择的会话成功！会话ID：{}", selectedSessionID), Command_Core::MessageTarget::CurrentChannel);
                                auto tmp = context.GetUserByName(context.GetCallingUserName());
                                Command_Core::User user(tmp.nickName, tmp.channelID, context.GetServerID(), tmp.clientID, tmp.uniqueID);
                                
                                auto method = Sessions::SessionFetchMethod::Index;
                                manager->UserUnselectSession(*(manager->GetSession(method, std::to_string(selectedSessionID)).value_or(nullptr)), user);
                                break;
                            }

                            auto method = FetchMethodMap.at(term.args[0]);
                            
                            //获取会话
                            auto session_ = manager->GetSession(method, term.args[1]);

                            if(!session_.has_value())
                            {
                                context.SendResult("SessionsSelect: 获取会话失败！", Command_Core::MessageTarget::CurrentChannel);
                                return "SessionsSelect: 获取会话失败！";
                            }

                            //获取并构造一个用户
                            auto tmp = context.GetUserByName(context.GetCallingUserName());
                            Command_Core::User user(tmp.nickName, tmp.channelID, context.GetServerID(), tmp.clientID, tmp.uniqueID);

                            manager->UserUnselectSession(*session_.value(), user);
                        }
                        break;
                    case TermType::UserJoin:
                        {
                            std::vector<Command_Core::User> users;
                            Sessions::SessionTemp::Session * workingSession;

                            if(UserSelectedSession == 0)
                            {
                                auto method = Sessions::SessionFetchMethod::Creator;
                                workingSession = manager->GetSession(method, context.GetCallingUserName()).value_or(nullptr);
                                if(workingSession == nullptr)
                                {
                                    context.SendResult("Sessions: 当前用户不是会话创建者或Session不存在！", Command_Core::MessageTarget::CurrentChannel);
                                    return "Sessions: 当前用户不是会话创建者或Session不存在！";
                                }
                            }
                            else
                            {
                                workingSession = WorkingSession;
                            }

                            //每一个arg是一个字符串类型的用户名
                            for(auto& user : term.args)
                            {
                                context.SendResult(std::format("查找用户：{}", user), Command_Core::MessageTarget::CurrentChannel);
                                auto tmp = context.GetUserByName(user);
                                if(tmp.uniqueID == "")
                                {
                                    context.SendResult(std::format("Sessions: 未查询到用户：{}！", user), Command_Core::MessageTarget::CurrentChannel);
                                    continue;
                                }
                                Command_Core::User user(tmp.nickName, tmp.channelID, context.GetServerID(), tmp.clientID, tmp.uniqueID);

                                //创建逻辑用户对象
                                users.push_back(user);
                                context.SendResult(std::format("Sessions: 添加用户 {} 成功！", user.name), Command_Core::MessageTarget::CurrentChannel);
                            }

                            manager->AddUser(workingSession->GetID(), users, context);
                        }
                        break;
                    case TermType::UserQuit:
                        {
                            std::vector<Command_Core::User> users;
                            Sessions::SessionTemp::Session * workingSession;
                            if(UserSelectedSession == 0)
                            {
                                auto method = Sessions::SessionFetchMethod::Creator;
                                workingSession = manager->GetSession(method, context.GetCallingUserName()).value_or(nullptr);
                                if(workingSession == nullptr)
                                {
                                    context.SendResult("Sessions: 当前用户不是会话创建者或Session不存在！", Command_Core::MessageTarget::CurrentChannel);
                                    return "Sessions: 当前用户不是会话创建者或Session不存在！";
                                }
                            }
                            else
                            {
                                workingSession = WorkingSession;
                            }
    
                            //每一个arg是一个字符串类型的用户名
                            for(auto& user : term.args)
                            {
                                context.SendResult(std::format("查找用户：{}", user), Command_Core::MessageTarget::CurrentChannel);
                                auto tmp = context.GetUserByName(user);
                                if(tmp.uniqueID == "")
                                {
                                    context.SendResult(std::format("Sessions: 未查询到用户：{}！", user), Command_Core::MessageTarget::CurrentChannel);
                                    continue;
                                }
                                Command_Core::User user(tmp.nickName, tmp.channelID, context.GetServerID(), tmp.clientID, tmp.uniqueID);

                                //创建逻辑用户对象
                                users.push_back(user);
                                context.SendResult(std::format("Sessions: 添加待删除的用户 {} 成功！", user.name), Command_Core::MessageTarget::CurrentChannel);
                            }

                            manager->RemoveUser(workingSession->GetID(), users, context);
                        }
                        break;
                    case TermType::AdminsSet: 
                        break;
                    case TermType::AdminsRemove: 
                        break;
                    case TermType::AdminsList: 
                        break;
                    case TermType::History:
                        {
                            //Arg0: 无：列出默认数量会话，Arg1：列出指定数量的会话。
                            const auto& List = manager->GetDiceSystem(context)->GetDiceEventListConst();
                            std::string result;
                            std::string time;

                            context.SendResult(std::format("历史记录总数: {}", List.size()), Command_Core::MessageTarget::CurrentChannel);
                            if(term.args.size() < 1)
                            {
                                //默认列出20条
                                size_t count = 20;
                                for(auto&& event : List)
                                {
                                    if(count-- == 0)
                                        break;
                                    
                                    //time
                                    time.clear();
                                    time = std::format("{}年{}月{}日{}时{}分{}秒", event->GetTimeInfo().tm_year + 1900, event->GetTimeInfo().tm_mon, event->GetTimeInfo().tm_mday, event->GetTimeInfo().tm_hour, event->GetTimeInfo().tm_min, event->GetTimeInfo().tm_sec);

                                    result += std::format("{} 在 {} 投掷出 ", context.GetUserNameByID(std::stoull(event->GetPlayerID())), time);
                                    result += std::format("{}\n", event->ParseExpressionStr());
                                    result += std::format("备注: {}\n", event->GetThrowLog().empty() ? "NULL" : event->GetThrowLog());
                                }
                            }
                            else if(term.args.size() == 1)
                            {
                                auto count = std::stoi(term.args[0]);
                                for(auto&& event : List)
                                {
                                    if(count-- == 0)
                                        break;
                                    
                                    //time
                                    time.clear();
                                    time = std::format("{}年{}月{}日{}时{}分{}秒", event->GetTimeInfo().tm_year + 1900, event->GetTimeInfo().tm_mon, event->GetTimeInfo().tm_mday, event->GetTimeInfo().tm_hour, event->GetTimeInfo().tm_min, event->GetTimeInfo().tm_sec);

                                    result += std::format("{} 在 {} 投掷出 ", context.GetUserNameByID(std::stoull(event->GetPlayerID())), time);
                                    result += std::format("{}\n", event->ParseExpressionStr());
                                    result += std::format("备注: {}\n", event->GetThrowLog().empty() ? "NULL" : event->GetThrowLog());
                                }
                            }
                            else
                            {
                                return "SessionsHistory: 参数过多或过少！";
                            }

                            context.SendResult(std::format("历史记录获取成功:\n{}", result), Command_Core::MessageTarget::CurrentChannel);
                        }
                        break;
                    case TermType::SaveToFile:
                        {
                            GwongDongFileSystem::FileObject file(fs::current_path());
                            //file.WriteFile()
                        }
                        break;
                    default: 
                        break;
                }
            }

            //返回执行的结果
            return "执行成功！";
        }

    private:

        static inline std::unordered_map<std::string_view, TermType> commands = 
        {
        
            {"Create", TermType::Create},
            {"create", TermType::Create},
            {"New", TermType::Create},
            {"new", TermType::Create},
            {"创建会话", TermType::Create},
            {"新建会话", TermType::Create},
            {"新建", TermType::Create},

            {"End", TermType::End},
            {"end", TermType::End},
            {"Stop", TermType::End},
            {"stop", TermType::End},
            {"结束会话", TermType::End},
            {"结束", TermType::End},
            {"停止会话", TermType::End},
            {"停止", TermType::End},


            {"Status", TermType::Status},
            {"status", TermType::Status},
            {"St", TermType::Status},
            {"st", TermType::Status},
            {"状态", TermType::Status},
            {"查询状态", TermType::Status},

            {"Info", TermType::Info},
            {"info", TermType::Info},
            {"I", TermType::Info},
            {"i", TermType::Info},
            {"查询信息", TermType::Info},
            {"信息", TermType::Info},
            {"会话信息", TermType::Info},

            {"List", TermType::List},
            {"list", TermType::List},
            {"ls", TermType::List},
            {"会话列表", TermType::List},
            {"查询列表", TermType::List},

            {"Search", TermType::Search},
            {"search", TermType::Search},
            {"查询", TermType::Search},
            {"搜索会话", TermType::Search},

            {"Activate", TermType::Activate},
            {"activate", TermType::Activate},
            {"act", TermType::Activate},
            {"激活会话", TermType::Activate},
            {"激活", TermType::Activate},

            {"Rest", TermType::Rest},
            {"rest", TermType::Rest},
            {"Pause", TermType::Rest},
            {"pause", TermType::Rest},
            {"暂停会话", TermType::Rest},
            {"暂停", TermType::Rest},

            {"Select", TermType::Select},
            {"select", TermType::Select},
            {"选择会话", TermType::Select},
            {"选择", TermType::Select},

            {"UnSelect", TermType::UnSelect},
            {"unselect", TermType::UnSelect},
            {"取消选择", TermType::UnSelect},

            {"UserJoin", TermType::UserJoin},
            {"userjoin", TermType::UserJoin},
            {"Join", TermType::UserJoin},
            {"Add", TermType::UserJoin},
            {"Usr", TermType::UserJoin},
            {"添加用户", TermType::UserJoin},
            {"加入用户", TermType::UserJoin},

            {"UserQuit", TermType::UserQuit},
            {"userquit", TermType::UserQuit},
            {"DeleteUser", TermType::UserQuit},
            {"Delete", TermType::UserQuit},
            {"Quit", TermType::UserQuit},
            {"退出用户", TermType::UserQuit},
            {"退出", TermType::UserQuit},
            {"删除用户", TermType::UserQuit},
            {"删除", TermType::UserQuit},
            {"移除用户", TermType::UserQuit},
            {"移除", TermType::UserQuit},
            {"用户退出", TermType::UserQuit},


            {"AdminsSet", TermType::AdminsSet},
            {"adminset", TermType::AdminsSet},
            {"SetAdmins", TermType::AdminsSet},
            {"setadmins", TermType::AdminsSet},
            {"设置管理员", TermType::AdminsSet},
            {"管理员", TermType::AdminsSet},

            {"AdminsRemove", TermType::AdminsRemove},
            {"adminsremove", TermType::AdminsRemove},
            {"RemoveAdmins", TermType::AdminsRemove},
            {"removeadmins", TermType::AdminsRemove},
            {"移除管理员", TermType::AdminsRemove},
            {"移除管理", TermType::AdminsRemove},

            {"AdminsList", TermType::AdminsList},
            {"adminslist", TermType::AdminsList},
            {"ListAdmins", TermType::AdminsList},
            {"listadmins", TermType::AdminsList},
            {"管理员列表", TermType::AdminsList},
            {"查询管理员", TermType::AdminsList},

            {"UserList", TermType::UserList},
            {"userlist", TermType::UserList},
            {"ListUsers", TermType::UserList},
            {"listusers", TermType::UserList},
            {"用户列表", TermType::UserList},

            {"Bind", TermType::Bind},
            {"bind", TermType::Bind},
            {"绑定", TermType::Bind},

            {"History", TermType::History},
            {"history", TermType::History},
            {"历史", TermType::History},
            {"查看历史", TermType::History},
            {"查询历史", TermType::History},

            {"CleanHistory", TermType::CleanHistory},
            {"cleanhistory", TermType::CleanHistory},
            {"清除历史", TermType::CleanHistory},
            {"删除历史记录", TermType::CleanHistory},
            {"删除历史", TermType::CleanHistory},

            {"Set", TermType::Set},
            {"set", TermType::Set},
            {"设置", TermType::Set},
            {"修改设置", TermType::Set},

            {"Export", TermType::Export},
            {"export", TermType::Export},
            {"导出会话数据", TermType::Export},
            {"导出会话记录", TermType::Export},

            {"Import", TermType::Import},
            {"import", TermType::Import},
            {"导入会话记录", TermType::Import},
            {"导入会话数据", TermType::Import},

            {"Help", TermType::Help},
            {"help", TermType::Help},
            {"GetHelp", TermType::Help},
            {"gethelp", TermType::Help},
            {"帮助", TermType::Help},
            {"获取帮助", TermType::Help},
            {"使用帮助", TermType::Help},
            {"查询帮助", TermType::Help},
            {"使用方法", TermType::Help},
            {"使用说明", TermType::Help},
            {"帮助文档", TermType::Help},

            {"Others", TermType::Others}
        
        };
        
    };
}







namespace Sessions
{
    void RegisterSessionsCommand(Command_Core::ICommandRegistry& registry);
}

