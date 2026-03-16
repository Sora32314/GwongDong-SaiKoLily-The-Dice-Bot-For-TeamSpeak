#pragma once
#include <deque>
#include <random>
#include <string_view>
#include <CommandCore.hpp>


namespace SaiKoLily
{
    namespace DiceSystem
    {
        class Dice;

        class DiceConfig;

        class DiceEventImpl;


        //骰子事件接口类
        class IDiceEvent
        {
        public:
            IDiceEvent() = default;
            virtual ~IDiceEvent() = default;

            //禁止拷贝函数
            IDiceEvent(const IDiceEvent&) = delete;
            IDiceEvent& operator=(const IDiceEvent&) = delete;

            //允许移动
            IDiceEvent(IDiceEvent&&) = default;
            IDiceEvent& operator=(IDiceEvent&&) = default;

        public:
            //成员函数
            
            //getter
            virtual void RollDiceEvent() = 0;
            virtual const std::vector<int64_t>& GetResultsList() = 0;
            virtual std::vector<int64_t> GetPointsList() = 0;
            virtual uint32_t GetDiceCount() = 0;
            virtual std::vector<int64_t> GetFaces() = 0;
            virtual std::string GetPlayerID() = 0;
            virtual std::tm GetTimeInfo() = 0;
            virtual int64_t GetPointsInTotal() = 0;
            virtual int64_t GetOffset() = 0;
            virtual const std::vector<std::vector<Dice>>& GetDiceTerms() const = 0;
            virtual const std::vector<std::pair<const DiceConfig, uint32_t>>& GetConfigTerms() const = 0;
            virtual const std::string GetThrowLog() const = 0;

            //setter
            virtual void SetOffset(int64_t _offset) = 0;
            virtual void SetDiceTerms(std::vector<std::vector<Dice>>&& _dice_terms) = 0;
            virtual void AddDiceTerms(std::vector<Dice>&& _dice_terms) = 0;
            virtual void SetThrowLog(std::string_view log) = 0;

            //解析器
            virtual std::string ParseExpressionStr() = 0;

            //获取详情
            virtual std::vector<std::vector<int64_t>> GetTermDetails() const = 0;
        };

        //骰子系统接口类
        class IDiceSystem
        {
        public:
            virtual ~IDiceSystem() = default;
            IDiceSystem() = default;


            //禁止拷贝与赋值
            IDiceSystem(const IDiceSystem&) = delete;
            IDiceSystem& operator=(const IDiceSystem&) = delete;

            //允许移动
            IDiceSystem(IDiceSystem&&) = default;;
            IDiceSystem& operator=(IDiceSystem&&) = default;

            //成员函数
            virtual int64_t RollDice(int64_t& face, std::string_view player = "Unknow") = 0;

            //roll多个自由类型骰子
            virtual int64_t RollDice(std::vector<std::pair<DiceConfig, uint32_t>>& configs, std::string_view player = "Unknow", int64_t offset = 0) = 0;

            // //获取结果
            virtual int64_t GetResult() = 0;
            virtual int64_t GetLastResult() = 0;
            virtual IDiceEvent* GetThrow() = 0;
            virtual IDiceEvent* GetLastEvent() = 0;

            // virtual DiceEvent* FindThrowByName();
            // virtual DiceEvent* FindThrowByFrequency();
            // virtual std::vector<DiceEvent>& GetDiceEventList();
            // virtual std::vector<DiceEvent>& GetDiceEventListByName();
            virtual const std::deque<std::shared_ptr<IDiceEvent>>& GetDiceEventListConst() = 0;
            virtual const std::vector<std::shared_ptr<IDiceEvent>>& GetDiceEventListByNameConst(std::string_view playerID) = 0;


            //============记录点数信息============h
            // virtual int64 PointsAvg();
            // virtual int64 PointsMid();

            //获取历史记录等功能函数

            //按玩家ID查询顺序记录，禁止拷贝，返回纯引用。
            virtual const std::map<std::string, std::vector<std::shared_ptr<IDiceEvent>>>& GetPlayerDiceList() const = 0;
            virtual void SetGlobalPlayerDiceList(std::string playerID, std::vector<std::shared_ptr<IDiceEvent>>&& player_dices_lists) = 0;      //danger
            virtual std::vector<std::shared_ptr<IDiceEvent>>& GetPlayerDiceListByID(std::string_view playerID) = 0;

            //本局历史投掷顺序记录，禁止拷贝，返回纯引用。
            virtual const std::deque<std::shared_ptr<IDiceEvent>>& GetGlobalDiceList() const = 0;
            virtual void SetGlobalDiceList(std::deque<std::shared_ptr<IDiceEvent>>&& global_dices_lists) = 0;       //danger
            virtual std::deque<std::shared_ptr<IDiceEvent>>& GetGlobalDiceList() = 0;       //danger


            //RNG
            virtual std::mt19937_64& GetRNG() = 0;
            virtual std::seed_seq& GetSeedSeq() = 0;
        };

        //抽象工厂
        class IDiceSystemFactory
        {
        public:
            virtual ~IDiceSystemFactory() = default;
            virtual std::unique_ptr<SaiKoLily::DiceSystem::IDiceSystem> CreateDiceSystem() = 0;
        };


        IDiceSystemFactory* GetDiceSystemFactory();
        void SetDiceSystemFactory(IDiceSystemFactory* factory);
    }

    class IDiceContextProvider
    {
    public:
        virtual ~IDiceContextProvider() = default;
        
        virtual SaiKoLily::DiceSystem::IDiceSystem* GetDiceSystem(const Command_Core::ICommandContext& context) = 0;
    };
    void SetDiceContextProvider(IDiceContextProvider* provider);
    IDiceContextProvider* GetDiceContextProvider();
    
}


namespace Sessions::SessionTemp
{
    class ISession
    {
    public:
        virtual ~ISession() = default;
        ISession() = default;

        //禁止拷贝
        ISession(const ISession&) = delete;
        ISession& operator=(const ISession&) = delete;

        //获取会话的骰子系统
        virtual std::unique_ptr<SaiKoLily::DiceSystem::IDiceSystem>& GetDiceSystem() = 0;
    };
}

namespace Sessions
{
    enum class SessionFetchMethod;
}

namespace Sessions::SessionManagerTemp
{
    //ISessionManager
    class ISessionManager
    {
    public:
        virtual ~ISessionManager() = default;
        ISessionManager() = default;

        //禁止拷贝
        ISessionManager(const ISessionManager&) = delete;
        ISessionManager& operator=(const ISessionManager&) = delete;

        //允许移动
        ISessionManager(ISessionManager&&) = default;;
        ISessionManager& operator=(ISessionManager&&) = default;

        //获取会话
        virtual std::optional<Sessions::SessionTemp::ISession*> GetSession(Sessions::SessionFetchMethod& method, std::string_view arg) = 0;

        //获取用户选择的Session
        virtual const std::unordered_map<std::string, ID>& GetSelectionOfSession() const = 0;
    };
}



