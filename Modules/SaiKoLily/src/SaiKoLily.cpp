#include "SaiKoLily.hpp"
#include <CommandCore.hpp>
#include <SaiKoLilyInterface.hpp>


namespace SaiKoLily
{
    //初始化阶段
    static SaiKo_LogCallback s_logCallback = nullptr;
    void SetLogCallback(SaiKo_LogCallback callback)
    {
        s_logCallback = callback;
    }


    namespace DiceSystem
    {
        //============DiceConfig类实现============
        DiceConfig::DiceConfig(int64_t _faces, int64_t _faces_min, int64_t _offset, std::string_view _dice_name, std::string_view _dice_description, bool _sign) : faces(_faces), faces_min(_faces_min), offset(_offset), sign(_sign)
        {
            if(!_dice_name.empty())
            {
                dice_name = std::move(_dice_name);
            }
            else
            {
                dice_name = std::format("The {} dice D{}{}{}", sign ? "positive" : "negative", faces, offset > 0 ? "+" : "-" , offset);
            }
            
            if(!_dice_description.empty())
            {
                dice_description = std::move(_dice_description);
            }
            else
            {
                dice_description = std::format("The Dice which is D{}{}{}, min face = {}.", sign ? "positive" : "negative", faces, offset > 0 ? "+" : "-" ,offset, faces_min);
            }
        }

        //============Dice类实现============
        Dice::Dice(std::shared_ptr<const DiceConfig> _dice_config, bool _sign) : dice_config(_dice_config), sign(_sign) {}
        Dice::Dice(int64_t faces, int64_t _faces_min, bool _sign) : dice_config(std::make_shared<DiceConfig>(faces, _faces_min, 0, "", "", _sign)), sign(_sign) {};

        Dice::Dice(Dice && other) noexcept :
        dice_config(std::move(other.dice_config)),
        points(std::move(other.points)),
        sign(std::move(other.sign))
        {
            other.dice_config = nullptr;
            other.points = 0;
            other.sign = true;
        }
        

        int64_t Dice::Roll(std::mt19937_64 &rng)
        {
            std::uniform_int_distribution<int64_t> dist(dice_config->faces_min, dice_config->faces);
            points = dist(rng);
            return points;
        }

        int64_t Dice::GetPoints() const
        {
            return points;
        }

        bool Dice::GetSign() const
        {
            return sign;
        }

        std::shared_ptr<const DiceConfig> Dice::GetDiceConfig() const
        {
            return dice_config;
        }

        bool Dice::IsConstant() const
        {
            if(dice_config->faces < dice_config->faces_min)
                return true;

            return false;
        }

        void Dice::SetSign(bool _sign)
        {
            sign = _sign;
        }

        //============RollDice类实现============
        
        // DiceEventImpl::DiceEventImpl(std::vector<DiceConfig>& _configs, std::seed_seq& _seq, std::mt19937_64& _rng, std::string_view _player) : rng(_rng), dice_count(_configs.size())
        // {
        //     //初始化时间信息
        //     auto now = std::chrono::system_clock::now();
        //     std::time_t now_time_t = std::chrono::system_clock::to_time_t(now);
        //     time_info = *(std::localtime(&now_time_t));
        //     configs = _configs;
        //     playerID = _player;
        // }

        DiceEventImpl::DiceEventImpl(std::vector<int64_t>& _faces, std::seed_seq& _seq, std::mt19937_64& _rng, std::string_view _player) : rng(_rng), dice_count(_faces.size())
        {
            //初始化时间信息
            time_info = std::chrono::system_clock::now();

            faces = _faces;
            faces_min.resize(faces.size(), 1);
            playerID = _player;
        }

        DiceEventImpl::DiceEventImpl(std::vector<int64_t>& _faces, std::vector<int64_t>& _faces_min, std::seed_seq& _seq, std::mt19937_64& _rng, std::string_view _player) : rng(_rng), dice_count(_faces.size())
        {
            //初始化时间信息
            time_info = std::chrono::system_clock::now();

            faces_min = _faces_min;
            faces = _faces;
            playerID = _player;
        }

        DiceEventImpl::DiceEventImpl(std::vector<std::pair<DiceConfig, uint32_t>>& _term_configs, std::seed_seq &_seq, std::mt19937_64 &_rng, std::string_view _player, int64_t constant_offset) : rng(_rng), dice_count(_term_configs.size())
        {
            time_info = std::chrono::system_clock::now();

            playerID = _player;

            for(auto&& term_config : _term_configs)
            {
                config_terms.emplace_back(term_config.first, term_config.second);
            }
        }

        void DiceEventImpl::RollDiceEvent()
        {
            for(auto&& config : config_terms)
            {
                s_logCallback(std::format("ROLL DICE EVENT正在投掷骰子：{}，面数：{}，最小面数：{}，偏移值：{}，符号：{}，个数: {}", config.first.dice_name, config.first.faces, config.first.faces_min, config.first.offset, config.first.sign ? "正" : "负", config.second), Plugin_Logs::logLevel::info, true);
                break;
            }

            if(config_terms.empty())
            {
                s_logCallback("进入到错误的分支：没有骰子可以被投掷并且无任何常量。", Plugin_Logs::logLevel::err, true);
                return;
            }

            //使用指针来进行选举。
            

            for(auto&& config : config_terms)
            {
                #ifdef _DEBUG
                    s_logCallback(std::format("正在投掷骰子：{}，面数：{}，最小面数：{}，偏移值：{}，符号：{}", config.first.dice_name, config.first.faces, config.first.faces_min, config.first.offset, config.first.sign ? "正" : "负"), Plugin_Logs::logLevel::info, true);
                #endif
                
                std::vector<Dice> dice_record;

                if(config.first.faces >= config.first.faces_min)
                {
                    
                    for(uint32_t i = 0; i < config.second; i++)
                    {
                        auto dice = Dice(std::make_shared<const DiceConfig>(config.first), config.first.sign);
                        dice.Roll(rng);

                        points.emplace_back(dice.GetPoints());
                        dice_record.emplace_back(std::move(dice));
                        #ifdef _DEBUG
                            s_logCallback(std::format("投掷结果：{}", dice_record.back().GetDiceConfig()->dice_name), Plugin_Logs::logLevel::info, true);
                        #endif
                    }
                    AddDiceTerms(std::move(dice_record));
                }
                else
                {
                    for(uint32_t i = 0; i < config.second; i++)
                    {
                        auto dice = Dice(std::make_shared<const DiceConfig>(config.first), config.first.sign);
                        points.emplace_back(dice.GetPoints());
                        dice_record.emplace_back(std::move(dice));
                        #ifdef _DEBUG
                            s_logCallback(std::format("投掷结果：{}", dice_record.back().GetDiceConfig()->dice_name), Plugin_Logs::logLevel::info, true);
                        #endif
                    }
                    AddDiceTerms(std::move(dice_record));
                }
            }
            
            

            for(auto&& config : GetDiceTerms())
            {
                s_logCallback(std::format("在ROLLDiceEvent中获取骰子项：{}，面数：{}，最小面数：{}，偏移值：{}，符号：{}，点数：{}", config.begin()->GetDiceConfig()->dice_name, config.begin()->GetDiceConfig()->faces, config.begin()->GetDiceConfig()->faces_min, config.begin()->GetDiceConfig()->offset, config.begin()->GetSign() ? "正" : "负", config.begin()->GetPoints()), Plugin_Logs::logLevel::info, true);
            }
        }

        const std::vector<int64_t>& DiceEventImpl::GetResultsList()
        {
            return points;
        }

        std::vector<int64_t> DiceEventImpl::GetPointsList()
        {
            return points;
        }

        uint32_t DiceEventImpl::GetDiceCount()
        {
            return dice_count;
        }

        std::vector<int64_t> DiceEventImpl::GetFaces()
        {
            return faces;
        }

        std::string DiceEventImpl::GetPlayerID()
        {
            return playerID;
        }

        TimePoint DiceEventImpl::GetTimeInfo()
        {
            return time_info;
        }

        int64_t DiceEventImpl::GetPointsInTotal()
        {
            int64_t points = 0;
            for(auto&& term : dice_terms)
            {
                for(auto&& dice : term)
                {
                    if(dice.IsConstant())
                    {
                        points += dice.GetSign() ? dice.GetDiceConfig()->offset : 0 - dice.GetDiceConfig()->offset;
                        continue;
                    }
                    points += dice.GetSign() ? dice.GetPoints() : 0 - dice.GetPoints();
                }
            }

            return points;
        }

        int64_t DiceEventImpl::GetOffset()
        {
            return offset;
        }

        const std::vector<std::vector<Dice>>& DiceEventImpl::GetDiceTerms() const
        {
            return dice_terms;
        }

        const std::vector<std::pair<const DiceConfig, uint32_t>> &DiceEventImpl::GetConfigTerms() const
        {
            return config_terms;
        }

        const std::string DiceEventImpl::GetThrowLog() const
        {
            return throw_log;
        }

        void DiceEventImpl::SetDiceTerms(std::vector<std::vector<Dice>>&& _dice_terms)
        {
            dice_terms = std::move(_dice_terms);
        }

        void DiceEventImpl::AddDiceTerms(std::vector<Dice> &&_dice_terms)
        {
            dice_terms.push_back(std::move(_dice_terms));
        }

         void DiceEventImpl::SetThrowLog(std::string_view log)
        {
            throw_log = log;
        }

        void DiceEventImpl::SetOffset(int64_t _offset)
        {
            offset = _offset;
        }

        std::string DiceEventImpl::ParseExpressionStr()
        {
            //基于骰子记录反向推导表达式字符串
            std::string expression_str;
            bool first_term = true;

            for(auto&& dices_per_throw : dice_terms)
            {
                if(first_term)
                {   
                    if(dices_per_throw.front().IsConstant())
                    {
                        expression_str += std::format("{}", dices_per_throw.front().GetDiceConfig()->offset);
                    }
                    else
                    {
                        expression_str += std::format("{}d{}", dices_per_throw.size(), dices_per_throw.front().GetDiceConfig()->faces);
                        expression_str += "[";
                        for(auto&& dice : dices_per_throw)
                        {
                            if(&dice == &dices_per_throw.back())
                            {
                                expression_str += std::format("{}", dice.GetPoints());
                                break;
                            }
                            expression_str += std::format("{}, ", dice.GetPoints());
                        }
                        expression_str += "]";
                    }
                    first_term = false;
                }
                else
                {
                    if(dices_per_throw.front().IsConstant())
                    {
                        expression_str += std::format(" {}{}", dices_per_throw.front().GetSign() ? "+" : "-", dices_per_throw.front().GetDiceConfig()->offset);
                    }
                    else
                    {
                        expression_str += std::format(" {} {}d{}", dices_per_throw.front().GetSign() ? "+" : "-", dices_per_throw.size(), dices_per_throw.front().GetDiceConfig()->faces);
                        expression_str += "[";
                        for(auto&& dice : dices_per_throw)
                        {
                            if(&dice == &dices_per_throw.back())
                            {
                                expression_str += std::format("{}", dice.GetPoints());
                                break;
                            }
                            expression_str += std::format("{}, ", dice.GetPoints());
                        }
                        expression_str += "]";
                    }
                }
            }
           
            expression_str += std::format("\r\t 最终结果为: {}。", GetPointsInTotal());

            return expression_str;
        }

        std::vector<std::vector<int64_t>> DiceEventImpl::GetTermDetails() const
        {
            //TODO: 获取表达式字符串带实现
            return std::vector<std::vector<int64_t>>();
        }

        size_t DiceEventImpl::GetSize() const
        {
            return sizeof(*this);
        }

        //============RollDice类实现============

        int64_t DiceSystemImpl::RollDice(int64_t &face, std::string_view player) 
        {
            //尝试为新的用户构建空的存储结构
            std::vector<std::shared_ptr<IDiceEvent>> empty_list;
            player_dices_lists.try_emplace(player.data(), empty_list);
            //构建vector用以匹配.emplace_back调用的构造参数
            std::vector<int64_t> faces{face};

            //创建骰子事件集
            player_dices_lists.at(std::string(player)).push_back(std::make_shared<DiceEventImpl>(faces, seed_seq, g_rng, player));
            auto& dice_event = player_dices_lists.at(player.data()).back();
            dice_event->RollDiceEvent();

            global_dices_lists.push_back(dice_event);

            //快速返回骰子结果
            return dice_event->GetPointsInTotal();
        }

        int64_t DiceSystemImpl::RollDice(std::vector<std::pair<DiceConfig, uint32_t>> &configs, std::string_view player, int64_t offset) 
        {
            #ifdef _DEBUG
                s_logCallback(std::format("正在尝试投掷骰子，玩家ID：{}，骰子配置项数量：{}，偏移值：{}。", player, configs.size(), offset), Plugin_Logs::logLevel::info, true);
            #endif

            if(configs.empty())
            {
                s_logCallback("DiceSystem尝试投掷骰子，但骰子配置项列表为空！无法进行投掷！", Plugin_Logs::logLevel::err, true);
                return int64_t();
            }

            //尝试为新的用户构建空的存储结构
            std::vector<std::shared_ptr<IDiceEvent>> empty_list;
            player_dices_lists.try_emplace(player.data(), empty_list);

            //创建骰子事件集
            player_dices_lists.at(player.data()).push_back(std::make_shared<DiceEventImpl>(configs, seed_seq, g_rng, player));
            auto& dice_event = player_dices_lists.at(player.data()).back();

            dice_event->SetOffset(offset);
            dice_event->RollDiceEvent();

            global_dices_lists.push_back(dice_event);

            return dice_event->GetPointsInTotal();
        }

        int64_t DiceSystemImpl::GetResult() 
        {
            return global_dices_lists.back()->GetPointsInTotal();
        }

        int64_t DiceSystemImpl::GetLastResult()
        {
            if(global_dices_lists.size() > 1)
            {
                return global_dices_lists[global_dices_lists.size() - 1]->GetPointsInTotal();
            }

            return global_dices_lists.back()->GetPointsInTotal();
        }

        IDiceEvent* DiceSystemImpl::GetThrow() 
        {
            if(!global_dices_lists.empty())
            {
                return global_dices_lists.back().get();
            }
            return nullptr;
        }
        
        IDiceEvent* DiceSystemImpl::GetLastEvent() 
        {
            if(!global_dices_lists.empty())
            {
                return global_dices_lists.back().get();
            }
            return nullptr;
        }

        const std::deque<std::shared_ptr<IDiceEvent>>& DiceSystemImpl::GetDiceEventListConst()
        {
            return global_dices_lists;
        }

        const std::vector<std::shared_ptr<IDiceEvent>>& DiceSystemImpl::GetDiceEventListByNameConst(std::string_view playerID)
        {
            return player_dices_lists.at(std::string(playerID));
        }
    
    
        //按玩家ID查询顺序记录，禁止拷贝，返回纯引用。
        const std::map<std::string, std::vector<std::shared_ptr<IDiceEvent>>>& DiceSystemImpl::GetPlayerDiceList() const
        {
            return player_dices_lists;
        }
        void DiceSystemImpl::SetGlobalPlayerDiceList(std::string playerID, std::vector<std::shared_ptr<IDiceEvent>>&& player_dices_lists)
        {
            this->player_dices_lists.at(playerID) = std::move(player_dices_lists);
        }
        std::vector<std::shared_ptr<IDiceEvent>>& DiceSystemImpl::GetPlayerDiceListByID(std::string_view playerID)
        {
            return player_dices_lists.at(std::string(playerID));
        }

        //本局历史投掷顺序记录，禁止拷贝，返回纯引用。
        const std::deque<std::shared_ptr<IDiceEvent>>& DiceSystemImpl::GetGlobalDiceList() const
        {
            return global_dices_lists;
        }
        void DiceSystemImpl::SetGlobalDiceList(std::deque<std::shared_ptr<IDiceEvent>>&& global_dices_lists)
        {
            this->global_dices_lists = std::move(global_dices_lists);
        }
        std::deque<std::shared_ptr<IDiceEvent>>& DiceSystemImpl::GetGlobalDiceList()
        {
            return global_dices_lists;
        }

        std::mt19937_64& DiceSystemImpl::GetRNG()
        {
            return g_rng;
        }

        std::seed_seq& DiceSystemImpl::GetSeedSeq()
        {
            return seed_seq;
        }

    
    
        //抽象工厂类实现
        class DefaultDiceSystemFactory : public IDiceSystemFactory
        {
        public:
            virtual std::unique_ptr<SaiKoLily::DiceSystem::IDiceSystem> CreateDiceSystem() override
            {
                return std::make_unique<SaiKoLily::DiceSystem::DiceSystemImpl>();
            }
        };

        static IDiceSystemFactory* s_diceSystemFactory = nullptr;

        IDiceSystemFactory* GetDiceSystemFactory()
        {
            if(!s_diceSystemFactory)
            {
                s_diceSystemFactory = new DefaultDiceSystemFactory();
            }
            return s_diceSystemFactory;
        }

        void SetDiceSystemFactory(IDiceSystemFactory* factory)
        {
            s_diceSystemFactory = factory;
        }

    }
}

