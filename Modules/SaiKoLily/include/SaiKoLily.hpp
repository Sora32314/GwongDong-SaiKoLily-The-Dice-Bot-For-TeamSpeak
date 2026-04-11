#pragma once

#include <any>
#include <map>
#include <list>
#include <deque>
#include <memory>
#include <chrono>
#include <vector>
#include <string>
#include <random>
#include <loggings.hpp>
#include <CommandCore.hpp>
#include <SaiKoLilyInterface.hpp>


using uint64 = unsigned long long;
using TimePoint = std::chrono::system_clock::time_point;


namespace SaiKoLily
{
    using SaiKo_LogCallback = std::function<void(const std::string&, Plugin_Logs::logLevel, bool nowFlush)>;
    void SetLogCallback(SaiKo_LogCallback callback);

    namespace DiceSystem
    {
        class DiceConfig
        {
        public:
            DiceConfig(int64_t _faces = -1, int64_t _faces_min = 1, int64_t _offset = 0, std::string_view _dice_name = "骰子", std::string_view _dice_description = "一个N面的骰子。", bool sign = true);
        public:
            int64_t faces;
            int64_t faces_min;
            int64_t offset;
            bool sign = true;
            std::string dice_name;
            std::string dice_description;
        };

        // 预设骰子类型
        namespace DicePresets {
            inline const DiceConfig& STANDARD_D4() {
                static const DiceConfig instance(4, 1, 0, "四面骰");
                return instance;
            };
            inline const DiceConfig& STANDARD_D6()
            {
                static const DiceConfig instance(6, 1, 0, "六面骰");
                return instance;
            };
            inline const DiceConfig& STANDARD_D8()
            {
                static const DiceConfig instance(8, 1, 0, "八面骰");
                return instance;
            };
            inline const DiceConfig& STANDARD_D10()
            {
                static const DiceConfig instance(10, 1, 0, "十面骰");
                return instance;
            };
            inline const DiceConfig& STANDARD_D20()
            {
                static const DiceConfig instance(20, 1, 0, "二十面骰");
                return instance;
            };
            inline const DiceConfig& STANDARD_D100()
            {
                static const DiceConfig instance(100, 1, 0, "百面骰");
                return instance;
            };
        }

        class Dice
        {
        public:
            Dice(std::shared_ptr<const DiceConfig> _dice_config, bool _sign = true);
            Dice(int64_t faces, int64_t faces_min = 1, bool _sign = true);
            ~Dice() = default;

            //允许移动
            Dice(Dice&& other) noexcept;
            Dice& operator=(Dice&& other) noexcept = default;

            //禁止拷贝与赋值
            Dice(const Dice&) = delete;
            Dice& operator=(const Dice&) = delete;


        public:
            //成员函数
            int64_t Roll(std::mt19937_64& rng);

            //getter
            int64_t GetPoints() const;
            bool GetSign() const;
            std::shared_ptr<const DiceConfig> GetDiceConfig() const;
            bool IsConstant() const;

            //setter
            void SetSign(bool _sign);

        private:
            std::shared_ptr<const DiceConfig> dice_config;
            int64_t points;
            bool sign = true;
        };

        class DiceEventImpl : public IDiceEvent
        {
        public:
            //DiceEventImpl(std::vector<DiceConfig>& _configs, std::seed_seq& _seq, std::mt19937_64& _rng, std::string_view _player = "Unknow");
            DiceEventImpl(std::vector<int64_t>& _faces, std::seed_seq& _seq, std::mt19937_64& _rng, std::string_view _player = "Unknow");
            DiceEventImpl(std::vector<int64_t>& _faces, std::vector<int64_t>& _faces_min, std::seed_seq& _seq, std::mt19937_64& _rng, std::string_view _player = "Unknow");
            DiceEventImpl(std::vector<std::pair<DiceConfig, uint32_t>>& _term_configs, std::seed_seq& _seq, std::mt19937_64& _rng, std::string_view _player = "Unknow", int64_t constant_offset = 0);

            ~DiceEventImpl() = default;;

            //禁止拷贝函数
            DiceEventImpl(const DiceEventImpl&) = delete;
            DiceEventImpl& operator=(const DiceEventImpl&) = delete;

            //允许移动
            DiceEventImpl(DiceEventImpl&&) = default;
            DiceEventImpl& operator=(DiceEventImpl&&) = default;

        public:
            //成员函数
            void RollDiceEvent() override;
            const std::vector<int64_t>& GetResultsList() override;
            std::vector<int64_t> GetPointsList() override;
            uint32_t GetDiceCount() override;
            std::vector<int64_t> GetFaces() override;
            std::string GetPlayerID() override;
            TimePoint GetTimeInfo() override;
            int64_t GetPointsInTotal() override;
            int64_t GetOffset() override;
            const std::vector<std::vector<Dice>>& GetDiceTerms() const override;
            const std::vector<std::pair<const DiceConfig, uint32_t>>& GetConfigTerms() const override;
            const std::string GetThrowLog() const override;

            void SetOffset(int64_t offset) override;
            void SetDiceTerms(std::vector<std::vector<Dice>>&& _dice_terms) override;
            void AddDiceTerms(std::vector<Dice>&& _dice_terms) override;
            void SetThrowLog(std::string_view log) override;
            

            //解析器
            std::string ParseExpressionStr() override;
            
            //获取详情
            std::vector<std::vector<int64_t>> GetTermDetails() const override;

        private:

            //骰子项存储
            std::vector<std::vector<Dice>> dice_terms;  // <骰子列表>
            

            //存储
            std::vector<int64_t> faces;
            std::vector<int64_t> faces_min;
            std::vector<int64_t> points;
            std::vector<std::pair<const DiceConfig, uint32_t>> config_terms;   // <骰子数量, 骰子配置>，用于快速访问骰子配置项。

            //结果缓存，用于计算总点数时的快速访问。
            //总偏移值：为所有常数项的和。常数项会在构造函数中被加入到这个值之中，使用GetOffset()获取。
            int64_t offset = 0;
            int64_t total_points = 0;
            int64_t total_faces = 0;
            uint32_t dice_count = 0;

            //其他元信息
            TimePoint time_info;
            std::string playerID;
            //随机数生成器
            std::mt19937_64& rng;

            std::string throw_log;
        };

        class DiceSystemImpl : public IDiceSystem
        {
        public:
            DiceSystemImpl() : g_rng(seed_seq) {};
            ~DiceSystemImpl() = default;

            //禁止拷贝与赋值
            DiceSystemImpl(const DiceSystemImpl&) = delete;
            DiceSystemImpl& operator=(const DiceSystemImpl&) = delete;

            //允许移动
            DiceSystemImpl(DiceSystemImpl&&) = default;;
            DiceSystemImpl& operator=(DiceSystemImpl&&) = default;;

        public:
            //成员函数

            //============投掷骰子============
            //roll单个骰子
            int64_t RollDice(int64_t& face, std::string_view player = "Unknow") override;

            //roll多个自由类型骰子
            int64_t RollDice(std::vector<std::pair<DiceConfig, uint32_t>>& configs, std::string_view player = "Unknow", int64_t offset = 0) override;


            //快速骰子......
            int64_t FastRollDice(std::string_view player = "Unknow");
            int64_t FastRollDice(std::vector<int64_t>& faces, std::string_view player = "Unknow");
            int64_t FastRollDiceFour(std::string_view player = "Unknow");
            int64_t FastRollDiceSix(std::string_view player = "Unknow");
            int64_t FastRollDiceTen(std::string_view player = "Unknow");
            int64_t FastRollDiceHundred(std::string_view player = "Unknow");
            int64_t FastRollDiceAHundred(std::string_view player = "Unknow");

            //获取结果
            int64_t GetResult() override;
            int64_t GetLastResult() override;
            IDiceEvent* GetThrow() override;
            IDiceEvent* GetLastEvent() override;
            
            //DiceEventImpl* FindThrowByName();
            //DiceEventImpl* FindThrowByFrequency();
            
            const std::deque<std::shared_ptr<IDiceEvent>>& GetDiceEventListConst() override;
            const std::vector<std::shared_ptr<IDiceEvent>>& GetDiceEventListByNameConst(std::string_view playerID) override;

            //============记录点数信息============
            int64_t PointsAvg();
            int64_t PointsMid();


            //按玩家ID查询顺序记录，禁止拷贝，返回纯引用。
            const std::map<std::string, std::vector<std::shared_ptr<IDiceEvent>>>& GetPlayerDiceList() const override;
            void SetGlobalPlayerDiceList(std::string playerID, std::vector<std::shared_ptr<IDiceEvent>>&& player_dices_lists) override;      //danger
            std::vector<std::shared_ptr<IDiceEvent>>& GetPlayerDiceListByID(std::string_view playerID) override;

            //本局历史投掷顺序记录，禁止拷贝，返回纯引用。
            const std::deque<std::shared_ptr<IDiceEvent>>& GetGlobalDiceList() const override;
            void SetGlobalDiceList(std::deque<std::shared_ptr<IDiceEvent>>&& global_dices_lists) override;       //danger
            std::deque<std::shared_ptr<IDiceEvent>>& GetGlobalDiceList() override;       //danger


            //============随机数引擎获取============
            std::mt19937_64& GetRNG() override;
            std::seed_seq& GetSeedSeq() override;

            //============随机引擎热身============
            //为长期高质量随机数生成引擎准备的热身系统
            //void WarmUp();

        private:

            //按玩家ID查询顺序记录
            std::map<std::string, std::vector<std::shared_ptr<IDiceEvent>>> player_dices_lists;
            //本局历史投掷记录
            std::deque<std::shared_ptr<IDiceEvent>> global_dices_lists;
            //每个投掷系统一个随机种子生成器。
            std::random_device g_rd;
            std::seed_seq seed_seq = {g_rd(), g_rd(), g_rd(), g_rd(), g_rd(), g_rd(), g_rd(), g_rd()};
            std::mt19937_64 g_rng;
        };


    };






}





