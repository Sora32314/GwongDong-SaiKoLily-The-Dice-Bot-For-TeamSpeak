#pragma once
#include <CommandCore.hpp>
#include <SaiKoLily.hpp>



namespace SaiKoLily::DiceCommand
{
    void SetLogCallback(SaiKo_LogCallback callback);

    enum class TermType
    {
        Dice, Constant, Operator, Group
    };

    /*

    class ExpressionNode
    {
    public:
        ExpressionNode() = default;
        ~ExpressionNode() = default;

    public:
        union ExpressionTerm
        {
            struct
            {
                int faces;
                int faces_min;
                int count;
            } dice_data;

            struct
            {
                int constant;
            } constant_data;

            struct
            {
                char operator_char;
            } operator_data;

        } data;
    }

    
    */

    //表达式项结构 
    struct ExpressionTerm
    {
        TermType type;
        int sign;
        int count = 1;
        int faces = -1;
        int faces_min = 1;
        int constant = 0;
    };
    

    class RollCommand : public Command_Core::ICommandHandler
    {
    public:
        RollCommand() = default;

        std::string_view GetName() const override
        {
            return "roll";
        }
        std::string_view GetDescription() const override
        {
            return "骰子命令\"roll\"。支持表达式如：!r 2d6+1d4-1d4-3+3等。";
        }
        std::vector<std::string_view> GetAlias() const override
        {
            return {"r", "rd", "dice", "throw", "roll", "R", "RD", "DICE", "THROW", "ROLL"};
        }
        void Execute(Command_Core::ICommandContext& context) override;

    private:
        static std::vector<ExpressionTerm> ParseDiceExpression(std::string_view expr);

        std::string ExecuteDiceRoll(const std::vector<ExpressionTerm>& term, SaiKoLily::DiceSystem::IDiceSystem* dice_system, uint64 userID);

        SaiKoLily::DiceSystem::IDiceSystem* dice_system_;
    };

    void RegisterDiceCommands(Command_Core::ICommandRegistry& registry);
}


namespace SaiKoLily::CheckCommand
{
    void SetLogCallback(SaiKo_LogCallback callback);

    enum class TermType
    {
        List = 1, Last, Name, ID, Detail, Stats, Clear, Check, NULLCommand
    };

    struct ExpressionTerm
    {
        using Args = std::string;
        TermType type = TermType::NULLCommand;
        std::vector<Args> args;
    };


    class HistoryCommand : public Command_Core::ICommandHandler
    {
    public:
        HistoryCommand() = default;

        std::string_view GetName() const override
        {
            return "checkhistory";
        }
        std::string_view GetDescription() const override
        {
            return "查看骰子历史记录。";
        }
        std::vector<std::string_view> GetAlias() const override
        {
            return {"ch", "his", "history", "check", "CH", "HIS", "HISTORY", "CHECK", "CHECKHISTORY"};
        }
        
        void Execute(Command_Core::ICommandContext& context) override;

    private:
        std::vector<std::string> GetHistory(SaiKoLily::DiceSystem::IDiceSystem* dice_system, ID userID, size_t limit = 10);
        std::string GetHistoryString(const std::vector<std::string>& history, size_t limit = 10);
        std::string ExecuteHistory(SaiKoLily::DiceSystem::IDiceSystem* dice_system, const Command_Core::ICommandContext& context, const std::vector<ExpressionTerm>& terms);
        std::vector<ExpressionTerm> ParseHistoryExpression(std::vector<std::string_view> exprs, Command_Core::ICommandContext& context);
    };
    
    void RegisterHistoryCommands(Command_Core::ICommandRegistry& registry);

    // 参数解析字典（映射参数到TermType）
    extern const std::unordered_map<std::string_view, TermType> paramTypeMap;


}




