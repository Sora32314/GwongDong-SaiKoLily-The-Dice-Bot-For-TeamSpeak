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
    /**
     * @brief SaiKoLily 模块日志回调类型定义
     *
     * 回调函数用于将模块内部日志输出到外部日志系统。
     *
     * @param message 日志消息
     * @param level 日志等级
     * @param nowFlush 是否立即刷新
     */
    using SaiKo_LogCallback = std::function<void(const std::string&, Plugin_Logs::logLevel, bool nowFlush)>;

    /**
     * @brief 设置 SaiKoLily 模块的全局日志回调
     *
     * @param callback 日志回调函数
     */
    void SetLogCallback(SaiKo_LogCallback callback);

    namespace DiceSystem
    {

        /**
         * @brief 骰子配置类
         *
         * `DiceConfig` 描述了一种骰子的静态属性，包括面数、最小面值、名称和描述等。
         * 它通常被多个 `Dice` 对象共享（通过 `std::shared_ptr`），以减少内存占用。
         *
         * 该类同时也可用于表示常数项（通过设置 `faces = -1` 表示）。
         *
         * @see Dice, DicePresets
         */
        class DiceConfig
        {
        public:
            /**
             * @brief 构造一个骰子配置对象
             *
             * @param _faces 骰子面数，若为 -1 则表示常数项
             * @param _faces_min 最小面值，默认为 1
             * @param _offset 固定偏移量（常数项时使用）
             * @param _dice_name 骰子显示名称
             * @param _dice_description 骰子描述
             * @param sign 正负号（用于常数项）
             */
            DiceConfig(int64_t _faces = -1, int64_t _faces_min = 1, int64_t _offset = 0, std::string_view _dice_name = "骰子", std::string_view _dice_description = "一个N面的骰子。", bool sign = true);
        public:
            int64_t faces;
            int64_t faces_min;
            int64_t offset;
            bool sign = true;
            std::string dice_name;
            std::string dice_description;
        };

        /**
         * @brief 预设骰子配置常量
         *
         * 提供常用标准骰子（D4、D6、D8、D10、D20、D100）的预定义配置。
         * 所有函数返回静态 `DiceConfig` 对象的常量引用。
         */
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

        /**
         * @brief 单个骰子实例类
         *
         * `Dice` 代表一个具体的骰子对象，持有对其配置的共享引用，并存储一次投掷后的点数结果。
         * 它提供了 `Roll()` 方法用于生成随机点数，以及获取结果和正负号的访问器。
         *
         * 该类禁止拷贝，允许移动，以确保骰子事件中骰子项的唯一性。
         *
         * @see DiceConfig, DiceEventImpl
         */
        class Dice
        {
        public:
            /**
             * @brief 通过配置 DiceConfig 对象构造骰子
             *
             * @param _dice_config 骰子配置的共享指针
             * @param _sign 正负号，默认为正
             */
            Dice(std::shared_ptr<const DiceConfig> _dice_config, bool _sign = true);

            /**
             * @brief 通过面数和最小面值构造骰子（内部创建默认配置）
             *
             * @param faces 面数
             * @param faces_min 最小面值，默认 1
             * @param _sign 正负号
             */
            Dice(int64_t faces, int64_t faces_min = 1, bool _sign = true);
            ~Dice() = default;

            //允许移动
            Dice(Dice&& other) noexcept;
            Dice& operator=(Dice&& other) noexcept = default;

            //禁止拷贝与赋值
            Dice(const Dice&) = delete;
            Dice& operator=(const Dice&) = delete;


        public:
            /**
             * @brief 执行投掷，使用给定的随机数引擎生成点数
             *
             * @param rng 随机数生成器引擎引用
             * @return 本次投掷的点数（含正负号）
             */
            int64_t Roll(std::mt19937_64& rng);

            /**
             * @brief 获取最近一次投掷的点数（未经符号修正）
             * @return 点数绝对值
             */
            int64_t GetPoints() const;
            
            /**
             * @brief 获取该骰子的正负号
             * @return true 表示正号，false 表示负号
             */
            bool GetSign() const;
            
            /**
             * @brief 获取关联的骰子配置
             * @return 配置对象的共享指针
             */
            std::shared_ptr<const DiceConfig> GetDiceConfig() const;

            /**
             * @brief 判断此对象是否表示一个常数项
             * @return 若配置中 `faces == -1` 则为 true
             */
            bool IsConstant() const;

            /**
             * @brief 设置正负号
             * @param _sign 新的符号值
             */
            void SetSign(bool _sign);

        private:
            std::shared_ptr<const DiceConfig> dice_config;
            int64_t points;
            bool sign = true;
        };


        /**
         * @brief 骰子事件的具体实现类
         *
         * `DiceEventImpl` 是 `IDiceEvent` 接口的实现，封装了一次完整投掷事件的所有数据。
         * 它通过多种构造函数支持从简单面数列表或复杂配置项列表构建事件，并在内部维护
         * 骰子项结构、结果缓存、日志字符串等。
         *
         * 该类由 `DiceSystemImpl` 创建并返回，外部通常通过 `IDiceEvent` 接口操作。
         *
         * @see IDiceEvent, DiceSystemImpl
         */
        class DiceEventImpl : public IDiceEvent
        {
        public:
            //DiceEventImpl(std::vector<DiceConfig>& _configs, std::seed_seq& _seq, std::mt19937_64& _rng, std::string_view _player = "Unknow");

            /**
             * @brief 通过面数列表构造骰子事件
             *
             * @param _faces 骰子面数列表（每个面数对应一个骰子）
             * @param _seq 种子序列引用
             * @param _rng 随机数生成器引用
             * @param _player 投掷者 ID
             */
            DiceEventImpl(std::vector<int64_t>& _faces, std::seed_seq& _seq, std::mt19937_64& _rng, std::string_view _player = "Unknow");

            /**
             * @brief 通过面数列表和最小面值列表构造
             *
             * @param _faces 面数列表
             * @param _faces_min 最小面值列表（应与 faces 等长）
             * @param _seq 种子序列引用
             * @param _rng 随机数生成器引用
             * @param _player 投掷者 ID
             */
            DiceEventImpl(std::vector<int64_t>& _faces, std::vector<int64_t>& _faces_min, std::seed_seq& _seq, std::mt19937_64& _rng, std::string_view _player = "Unknow");
            
            /**
             * @brief 通过配置项列表构造（支持复杂表达式）
             *
             * @param _term_configs 配置项与数量的配对列表
             * @param _seq 种子序列引用
             * @param _rng 随机数生成器引用
             * @param _player 投掷者 ID
             * @param constant_offset 总常数偏移量
             */
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


            //获取类大小
            size_t GetSize() const override;

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


        /**
         * @brief 骰子系统的具体实现类
         *
         * `DiceSystemImpl` 实现了 `IDiceSystem` 接口，提供完整的骰子投掷、历史记录管理
         * 和随机数生成功能。每个 `DiceSystemImpl` 实例拥有独立的随机数引擎和种子序列，
         * 以及独立的全局投掷历史队列和按玩家分组的历史映射。
         *
         * 该类通常由 `IDiceSystemFactory` 创建，并与一个会话（Session）绑定。
         *
         * @see IDiceSystem, DiceSystemImpl, DiceEventImpl
         */
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
            /// @warning 下面的快速骰子函数不记录历史，仅返回结果，适用于需要大量快速投掷但不关心历史记录的场景。请谨慎使用。
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


            /// @brief 按玩家ID查询顺序记录，禁止拷贝，返回纯引用。
            const std::map<std::string, std::vector<std::shared_ptr<IDiceEvent>>>& GetPlayerDiceList() const override;
            
            /**
             * @brief 设置指定玩家的全局投掷事件列表
             * 
             * @param playerID 
             * @param player_dices_lists 
             * 
             * @warning 此方法直接替换玩家的事件列表，使用不当可能导致历史记录丢失或数据不一致。请确保在调用前备份原有数据。
             */
            void SetGlobalPlayerDiceList(std::string playerID, std::vector<std::shared_ptr<IDiceEvent>>&& player_dices_lists) override;      //danger
            
            /**
             * @brief 获取指定玩家的全局投掷事件列表。
             * 
             * @param playerID 
             * @return std::vector<std::shared_ptr<IDiceEvent>>& 
             * 
             * @see IDiceEvent, GetPlayerDiceList
             */
            std::vector<std::shared_ptr<IDiceEvent>>& GetPlayerDiceListByID(std::string_view playerID) override;

            /// @brief 获取全局投掷事件列表，禁止拷贝，返回纯引用。
            const std::deque<std::shared_ptr<IDiceEvent>>& GetGlobalDiceList() const override;

            /** 
             * @brief 设置全局投掷事件列表，禁止拷贝，接受右值引用以避免不必要的复制。
             * 
             * @param global_dices_lists 
             * 
             * @warning 此方法直接替换全局事件列表，使用不当可能导致历史记录丢失或数据不一致。请确保在调用前备份原有数据。
             */
            void SetGlobalDiceList(std::deque<std::shared_ptr<IDiceEvent>>&& global_dices_lists) override;       //danger

            /**
             * @brief 获取全局投掷事件列表，禁止拷贝，返回纯引用。
             * 
             * @return std::deque<std::shared_ptr<IDiceEvent>>& 
             * 
             * @warning 此方法返回全局事件列表的非 const 引用，允许外部修改全局历史记录。请谨慎使用，以避免数据不一致或线程安全问题。
             * 
             * @see IDiceEvent, GetGlobalDiceList
             */
            std::deque<std::shared_ptr<IDiceEvent>>& GetGlobalDiceList() override;       //danger


            //============随机数引擎获取============

            /**
             * @brief 获取当前骰子系统的随机数生成器
             * @return 随机数生成器的引用
             */ 
            std::mt19937_64& GetRNG() override;

            /**
             * @brief 获取当前骰子系统的随机数种子生成器
             * @return 随机数种子生成器的引用
             */
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





