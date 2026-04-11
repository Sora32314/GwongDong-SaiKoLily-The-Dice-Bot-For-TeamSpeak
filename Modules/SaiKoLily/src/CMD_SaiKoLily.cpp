#include <CMD_SaiKoLily.hpp>


namespace SaiKoLily::DiceCommand
{
    static SaiKo_LogCallback cmds_logCallback = nullptr;
    void SetLogCallback(SaiKo_LogCallback callback)
    {
        cmds_logCallback = callback;
    }

    //TODO:修改骰子命令，让她接受外部传入的DiceSystem
    void RollCommand::Execute(Command_Core::ICommandContext& context)
    {
        if(context.GetParamCount() < 1)
        {
            context.SendResult(std::format("用法： [!, .][roll, r, rd ...] [骰子表达式]。如： !r 2d10+1d8-1d4+3-1等。"), context.GetMessageTarget());
            context.Log(std::format("用法： [!, .][roll, r, rd ...] [骰子表达式]。如： !r 2d10+1d8-1d4+3-1等。"), Plugin_Logs::logLevel::warn, false);
            return;
        }

        //获取第一个参数（检测第一个参数是否为命令表达式）
        auto exprOpt = context.GetParam(0);
        if(!exprOpt)
        {
            context.SendResult(std::format("错误！缺少骰子表达式！"), context.GetMessageTarget());
            context.Log(std::format("错误！缺少骰子表达式！"), Plugin_Logs::logLevel::err, false);
            return;
        }

        //获取表达式选项
        std::string_view expr = *exprOpt;
        try
        {
            //1.解析表达式
            auto terms = ParseDiceExpression(expr);

            //2.获取骰子系统
            SaiKoLily::DiceSystem::IDiceSystem* dice_system = nullptr;
            auto temp = SaiKoLily::DiceSystem::DiceSystemImpl();
            if(SaiKoLily::GetDiceContextProvider())
            {
                cmds_logCallback("获取DiceSystem成功。", Plugin_Logs::logLevel::info, true);
                dice_system = SaiKoLily::GetDiceContextProvider()->GetDiceSystem(context);

                if(!dice_system)
                {
                    dice_system = &temp;
                }
                
                // for(const auto& res : dice_system->GetDiceEventListConst())
                // {
                //     cmds_logCallback(std::format("历史投掷记录：玩家:{}，点数:{}", res->GetPlayerID(), res->GetPointsInTotal()), Plugin_Logs::logLevel::info, true);
                // }

            }
            else
            {
                cmds_logCallback("获取DiceSystem失败。使用临时实例。", Plugin_Logs::logLevel::warn, true);
                dice_system = &temp;
            }

            //3.执行投掷
            std::string res = ExecuteDiceRoll(terms, dice_system, context.GetCallingUserID());

            //4.返回结果
            context.SendResult(res, context.GetMessageTarget());
        }
        catch(const std::exception& e)
        {
            //返回错误结果
            context.SendResult(std::format("执行投掷时出现错误: {}.", e.what()), context.GetMessageTarget());
            context.Log(std::format("执行投掷时出现错误: {}.", e.what()), Plugin_Logs::logLevel::err, false);
        }
        
    }

    //解析骰子命令执行主逻辑并拆分
    std::vector<ExpressionTerm> RollCommand::ParseDiceExpression(std::string_view expr)
    {
        std::vector<ExpressionTerm> terms;
        std::string current;
        //符号默认为正
        int sign = 1;
        bool nextSignPositive = true;

        auto addTerm = [&]()
        {
            if(current.empty())
            {
                return;
            }
            ExpressionTerm term;
            term.sign = sign;

            //判断是否是骰子项
            size_t dPos = current.find('d');
            if(dPos != std::string::npos)
            {
                term.type = TermType::Dice;

                //解析骰子数量
                if(dPos > 0)
                {
                    term.count = std::stoll(current.substr(0, dPos));
                }
                else
                {
                    //默认为一个骰子
                    term.count = 1;
                }

                //解析骰子面数
                term.faces = std::stoll(current.substr(dPos + 1));
            }
            else
            {
                //常数项
                term.type = TermType::Constant;
                term.constant = std::stoll(current);
            }

            terms.push_back(term);
            current.clear();
        };

        for(auto character : expr)
        {
            //忽略空格
            if(std::isspace(character))
            {
                continue;
            }
            
            if(character == '+' || character == '-')
            {
                //保存当前项
                addTerm();

                //设置下一项的符号
                nextSignPositive = (character == '+');
            }
            else
            {
                if(current.empty())
                {
                    sign = nextSignPositive ? 1 : -1;
                    //重置
                    nextSignPositive = true;
                }
                current += character;
            }

        }

        //添加最后一项
        addTerm();

#ifdef _DEBUG
        for(auto& term : terms)
        {
            cmds_logCallback(std::format("解析到项：类型={}, 符号={}, 数量={}, 面数/常数={}", static_cast<int>(term.type), term.sign, term.count, term.type == TermType::Dice ? term.faces : term.constant), Plugin_Logs::logLevel::info, true);
        }
#endif
        return terms;
    }


    std::string RollCommand::ExecuteDiceRoll(const std::vector<ExpressionTerm>& terms, SaiKoLily::DiceSystem::IDiceSystem* dice_system, uint64_t userID)
    {
        auto tmp = SaiKoLily::DiceSystem::DiceSystemImpl();
        if(!dice_system)
        {
            cmds_logCallback("DiceSystem为空，使用新的默认DiceSystem。", Plugin_Logs::logLevel::info, true);
            dice_system = &tmp;
        }

        #ifdef _DEBUG
        cmds_logCallback(std::format("ExecuteDiceRoll 开始执行骰子命令。"), Plugin_Logs::logLevel::info, true);
        #endif

        std::ostringstream result;
        int64_t total = 0;
        bool firstTerm = true;

        result << "投掷结果：";

        std::vector<std::pair<SaiKoLily::DiceSystem::DiceConfig, uint32_t>> configs;
        SaiKoLily::DiceSystem::DiceConfig config;


        for(const auto& term : terms)
        {
            switch(term.type)
            {
            case TermType::Dice:
                {
                    config.faces = term.faces;
                    config.faces_min = term.faces_min;
                    config.offset = term.constant;
                    config.sign = term.sign > 0;
                    config.dice_name = std::format("{}d{}", term.count, term.faces);
                    config.dice_description = std::format("投掷 {} 个 {}，点数范围：{}～{}，点数偏移：{}，符号：{}", term.count, config.dice_name, config.faces_min, config.faces, config.offset, config.sign ? "正" : "负");
                    configs.emplace_back(config, term.count);
                }
                break;
            case TermType::Constant:
                {
                    config.faces = -1;
                    config.faces_min = 1;
                    config.offset = term.constant;
                    config.sign = term.sign > 0;
                    config.dice_name = std::format("常数项");
                    config.dice_description = std::format("一个常数项，值为 {}。", term.constant);
                    configs.emplace_back(config, term.count);
                }
                break;
            }
        }

#ifdef _DEBUG
        cmds_logCallback(std::format("ExecuteDiceRoll 配置项："), Plugin_Logs::logLevel::info, true);

        for(auto&& config : configs)
        {
            cmds_logCallback(std::format("准备投掷：{}，面数：{}，最小面数：{}，偏移值：{}，符号：{}", config.first.dice_name, config.first.faces, config.first.faces_min, config.first.offset, config.first.sign ? "正" : "负"), Plugin_Logs::logLevel::info, true);
        }
#endif

        total = dice_system->RollDice(configs, std::to_string(userID), 0);

        auto lastEvent = dice_system->GetLastEvent();
        std::string signed_string;

#ifdef _DEBUG
        cmds_logCallback(std::format("投掷结果：{}", total), Plugin_Logs::logLevel::info, true);
#endif

        //构建结果字符串
        for(const auto& config : lastEvent->GetDiceTerms())
        {

#ifdef _DEBUG
            cmds_logCallback(std::format("ExecuteDiceRoll 投掷结果项：{}", config.at(0).GetDiceConfig()->dice_name), Plugin_Logs::logLevel::info, true);
#endif
            if(!firstTerm)
            {
                signed_string = config.front().GetSign() == true ? "+" : "-";

                if(config.back().IsConstant() == false)
                {
                    //解析为骰子项
                    result << signed_string << config.size() << "d" << config.back().GetDiceConfig()->faces << "[";
                    for(auto& dice : config)
                    {
                        if(&dice == &config.back())
                        {
                            result << dice.GetPoints();
                            break;
                        }
                        result << dice.GetPoints() << ", ";
                    }
                    result << "]";
                }
                else
                {
                    result << signed_string << config.front().GetDiceConfig()->offset;
                }
            }
            else
            {
                firstTerm = false;

                if(config.back().IsConstant() == false)
                {
                    
                    result << config.size() << "d" << config.back().GetDiceConfig()->faces << "[";
                    for(auto& dice : config)
                    {
                        if(&dice == &config.back())
                        {
                            result << dice.GetPoints();
                            break;
                        }
                        result << dice.GetPoints() << ", ";
                    }
                    result << "]";
                }
                else
                {
                    result << signed_string << config.front().GetDiceConfig()->offset;
                }
            }
        }


        result << "\n点数：" << lastEvent->GetPointsInTotal() << std::endl;

        return result.str();
    }

/*
    //根据解析并拆分好的参数执行骰子命令
    std::string RollCommand::ExecuteDiceRoll(const std::vector<ExpressionTerm>& terms, SaiKoLily::DiceSystem::IDiceSystem* dice_system, uint64 userID)
    {
        //获取DiceSystem，如果没有则创建一个默认的
        //TODO:重复检查错误了！

        auto tmp = SaiKoLily::DiceSystem::DiceSystemImpl();
        if (!dice_system)
        {   
            cmds_logCallback("DiceSystem为空，创建一个默认的DiceSystem。", Plugin_Logs::logLevel::info, true);
            dice_system = &tmp;
        }
        
        cmds_logCallback(std::format("ExecuteDiceRoll 开始执行骰子命令。"), Plugin_Logs::logLevel::info, true);
        
        std::ostringstream result;
        int64_t total = 0;
        bool firstTerm = true;

        result << "投掷结果：";

        //创建配置项
        std::vector<DiceSystem::DiceConfig> configs;

        //解析骰子项并调用执行投掷
        for(const auto& term : terms)
        {
            if(term.type == TermType::Dice)
            {
                //准备骰子配置
                std::vector<DiceSystem::DiceConfig> configs;
                configs.reserve(term.count);
                for(int i = 0; i < term.count; i++)
                {
                    configs.emplace_back(term.faces);
                    configs.back().sign = term.sign;
                }

                //执行投掷
                int64_t diceResult = dice_system->RollDice(configs, std::to_string(userID));
                int64_t actualResult = term.sign * diceResult;

                total += actualResult;

                //添加到结果字符串
                if(!firstTerm)
                {
                    result << (term.sign > 0 ? "+" : "-");
                }
                result << term.count << "d" << term.faces << "[";

                //获取详细结果
                auto lastEvent = dice_system->GetLastEvent();
                const auto& diceList = lastEvent->GetResultsList();
                for(size_t i = 0; i < diceList.size(); i++)
                {
                    if(i > 0)
                    {
                        result << "，";
                    }
                    result << diceList[i];
                }
                result << "]";
            }
            else
            {
                int constantValue = term.sign * term.constant;
                total += constantValue;

                if(!firstTerm)
                {
                    result << (term.sign > 0 ? "+" : "-");
                }
                result << term.constant;
                auto lastEvent = dice_system->GetLastEvent();
                lastEvent->SetOffset(lastEvent->GetOffset() + constantValue);
            }
            firstTerm = false;
        }

        result << "=" << total;
        cmds_logCallback(std::format("投掷结果：{}", total), Plugin_Logs::logLevel::info, true);
        return result.str();
    }
*/
    
    
    void RegisterDiceCommands(Command_Core::ICommandRegistry& registry)
    {
        cmds_logCallback(std::format("骰子命令初始化。"), Plugin_Logs::logLevel::info, false);
        registry.RegisterHandler(std::make_unique<RollCommand>());
        cmds_logCallback(std::format("骰子命令初始化结束。"), Plugin_Logs::logLevel::info, false);
    }

}




namespace SaiKoLily::CheckCommand
{

    static SaiKo_LogCallback checks_logCallback = nullptr;
    void SetLogCallback(SaiKo_LogCallback callback)
    {
        checks_logCallback = callback;
    }

    void HistoryCommand::Execute(Command_Core::ICommandContext& context)
    {
        //参数检查
        if(context.GetParamCount() <= 0)
        {
            checks_logCallback(std::format("参数错误。"), Plugin_Logs::logLevel::err, true);
            return;
        }

        //获取表达式
        auto exprOpt = context.GetParam(0);
        if(!exprOpt)
        {
            context.SendResult(std::format("错误！缺少查询历史命令表达式！"), context.GetMessageTarget());
            context.Log(std::format("错误！缺少查询历史命令表达式！"), Plugin_Logs::logLevel::err, false);
            return;
        }

        //获取参数
        size_t expr_count = context.GetParamCount();
        std::vector<std::string_view> exprs;
        for(auto i = 0; i < expr_count; i++)
        {
            exprs.push_back(context.GetParam(i).value());
        }

        SaiKoLily::DiceSystem::IDiceSystem* dice_system = nullptr;
        auto temp = SaiKoLily::DiceSystem::DiceSystemImpl();
        if(SaiKoLily::GetDiceContextProvider())
        {
            dice_system = SaiKoLily::GetDiceContextProvider()->GetDiceSystem(context);

            if(!dice_system)
            {
                dice_system = &temp;
            }
        }
        else
        {
            dice_system = &temp;
        }

        try
        {
            //解析命令
            auto terms = HistoryCommand::ParseHistoryExpression(exprs, context);

            //执行命令
            auto result = HistoryCommand::ExecuteHistory(dice_system, context, terms);

            //返回结果
            context.SendResult(result, context.GetMessageTarget());
        }
        catch(const std::exception& e)
        {
            context.SendResult(std::format("错误！{}", e.what()), context.GetMessageTarget());
            context.Log(std::format("错误！{}", e.what()), Plugin_Logs::logLevel::err, false);
        }
        
        return;
    }

    std::vector<ExpressionTerm> HistoryCommand::ParseHistoryExpression(std::vector<std::string_view> exprs, Command_Core::ICommandContext& context)
    {
        if(exprs.empty())
        {
            checks_logCallback(std::format("参数错误。"), Plugin_Logs::logLevel::err, true);
            return {};
        }
        std::vector<ExpressionTerm> terms;

        for(auto& expr : exprs)
        {
            if(paramTypeMap.contains(expr))
            {
                terms.emplace_back(paramTypeMap.at(expr), std::vector<ExpressionTerm::Args>{});
            }
            else if(!paramTypeMap.contains(expr) && !terms.empty())
            {
                terms.back().args.push_back(std::string(expr));
            }
            else
            {
                terms.emplace_back(TermType::NULLCommand, std::vector<ExpressionTerm::Args>{});
                checks_logCallback(std::format("命令 {} 不存在！", expr), Plugin_Logs::logLevel::warn, false);
            }
        }

        if(terms.empty())
        {
            context.Log("History: 命令为空！", Plugin_Logs::logLevel::warn, false);
            context.SendResult("History: 命令为空！", Command_Core::MessageTarget::CurrentChannel);
        }

        for(auto& term : terms)
        {
            if(term.type == TermType::NULLCommand)
            {
                context.Log(std::format("{} 命令为空！", static_cast<int>(term.type)), Plugin_Logs::logLevel::warn, false);
                context.SendResult(std::format("History: 命令 {} 为空！", static_cast<int>(term.type)), Command_Core::MessageTarget::CurrentChannel);
            }
            else
            {
                context.Log(std::format("History: 命令 {} 已被解析！", static_cast<int>(term.type)), Plugin_Logs::logLevel::info, false);
                context.SendResult(std::format("History: 命令 {} 已被解析！", static_cast<int>(term.type)), Command_Core::MessageTarget::CurrentChannel);
            }
        }

        context.Log(std::format("命令解析完毕"), Plugin_Logs::logLevel::info, true);

        return terms;

    }


    std::string HistoryCommand::ExecuteHistory(SaiKoLily::DiceSystem::IDiceSystem* dice_system, const Command_Core::ICommandContext& context, const std::vector<ExpressionTerm>& terms)
    {
        std::stringstream result;
        // bool firstTerm = true;
        
        auto list = dice_system->GetDiceEventListConst();

        if(list.empty())
        {
            result << "暂无历史记录\n";
            return result.str();
        }

        for(auto& term : terms)
        {
            if(term.type == TermType::NULLCommand)
            {
                continue;
            }

            switch(term.type)
            {
            case TermType::List:
                {
                    //TODO:支持常量以及分页等功能
                    result << "骰子历史记录列表: \n";

                    size_t count = 0;

                    if(term.args.size() == 1)
                    {
                        count = std::stoll(term.args[0]);

                        if(count > 100)
                        {
                            count = 100;
                        }
                    }
                    else
                    {
                        //默认显示20条
                        count = 20;
                    }

                    std::reverse(list.begin(), list.end());

                    for(auto& event : list)
                    {
                        if(count == 0)
                        {
                            break;
                        }

                        std::string time_str = std::format("{:%Y-%m-%d %H:%M:%S}", event->GetTimeInfo());
                        auto configs = event->GetConfigTerms();
                        auto results = event->GetPointsList();

                        std::string dice_expr_str;
                        int64_t total_without_constant = 0;
                        for(auto i = 0; i < configs.size(); i++)
                        {
                            dice_expr_str += std::format("{}d{}={} ", configs[i].first.sign ? "+" : "-", configs[i].first.faces, results[i]);
                            total_without_constant+= results[i];
                        }

                        dice_expr_str += std::format("排除常量部分总点数: {}\t", total_without_constant);

                        std::string player_name = context.GetUserNameByID(context.GetServerID(), std::stoull(event->GetPlayerID()))[0];

                        result << std::format("玩家:{} 投掷时间:{} 骰子点数（不含常量）:{} 最终总点数:{}", player_name, time_str, dice_expr_str, event->GetPointsInTotal()) << "\n";
                        count--;
                    }

                    return result.str();
                }
                break;
            case TermType::Check:
                result << "Check";
                break;
            case TermType::Clear:
                result << "Clear";
                break;

            case TermType::NULLCommand:
                continue;
                break;
            default:
                break;
            }
        }

        return "";
    }
    void RegisterHistoryCommands(Command_Core::ICommandRegistry& registry)
    {

        try
        {
            checks_logCallback(std::format("历史记录命令初始化开始..."), Plugin_Logs::logLevel::info, true);
            
            // 1. 先检查回调是否有效
            if (!checks_logCallback) {
                return;
            }
            
            // 2. 创建对象前
            checks_logCallback(std::format("准备创建 HistoryCommand 对象"), Plugin_Logs::logLevel::info, true);
            
            // 3. 创建对象
            std::unique_ptr<HistoryCommand> handler;
            try {
                handler = std::make_unique<HistoryCommand>();
                checks_logCallback(std::format("HistoryCommand 对象创建成功"), Plugin_Logs::logLevel::info, true);
            } catch (const std::exception& e) {
                checks_logCallback(std::format("创建 HistoryCommand 对象异常: {}", e.what()), Plugin_Logs::logLevel::err, true);
                return;
            } catch (...) {
                checks_logCallback("创建 HistoryCommand 对象未知异常", Plugin_Logs::logLevel::err, true);
                return;
            }
            
            // 4. 测试虚函数调用
            try {
                auto name = handler->GetName();
                checks_logCallback(std::format("GetName() 调用成功: {}", name), Plugin_Logs::logLevel::info, true);
            } catch (const std::exception& e) {
                checks_logCallback(std::format("GetName() 异常: {}", e.what()), Plugin_Logs::logLevel::err, true);
                return;
            }
            
            // 5. 注册
            try {
                registry.RegisterHandler(std::move(handler));
                checks_logCallback(std::format("HistoryCommand 注册成功"), Plugin_Logs::logLevel::info, true);
            } catch (const std::exception& e) {
                checks_logCallback(std::format("注册异常: {}", e.what()), Plugin_Logs::logLevel::err, true);
            }
            
            checks_logCallback(std::format("历史记录命令初始化结束"), Plugin_Logs::logLevel::info, true);
        }
        catch(const std::exception& e)
        {
            if (checks_logCallback) {
                checks_logCallback(std::format("历史记录命令初始化失败。{}", e.what()), Plugin_Logs::logLevel::err, true);
            }
        }
    
    }
        
        
    

    const std::unordered_map<std::string_view, TermType> paramTypeMap = {
        {"list", TermType::List},
        {"列表", TermType::List},
        {"ls", TermType::List},
        {"-li", TermType::List},
        {"-list", TermType::List},
        {"clear", TermType::Clear},
        {"清空", TermType::Clear},
        {"clean", TermType::Clear},
        {"detail", TermType::Detail},
        {"详细", TermType::Detail},
        {"-d", TermType::Detail},
        {"-detail", TermType::Detail},
        {"stats", TermType::Stats},
        {"统计", TermType::Stats},
        {"statistics", TermType::Stats},
        {"-s", TermType::Stats},
        {"-stats", TermType::Stats},
        {"last", TermType::Last},
        {"上次记录", TermType::Last},
        {"l", TermType::Last},
        {"-l", TermType::Last},
        {"ID", TermType::ID},
        {"Name", TermType::Name},
        {"check", TermType::Check},
        {"检定", TermType::Check},
        {"判定", TermType::Check}
    };
}
