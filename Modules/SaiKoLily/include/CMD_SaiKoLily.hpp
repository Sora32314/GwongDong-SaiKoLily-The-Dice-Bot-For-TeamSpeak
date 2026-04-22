#pragma once
#include <CommandCore.hpp>
#include <SaiKoLily.hpp>



namespace SaiKoLily::DiceCommand
{

    /**
     * @brief 设置骰子命令模块的日志回调
     *
     * 用于将命令执行过程中的日志信息转发至外部日志系统。
     *
     * @param callback 日志回调函数，接受消息字符串、日志等级和是否立即刷新的标志
     */
    void SetLogCallback(SaiKo_LogCallback callback);

    /**
     * @brief 骰子表达式词法单元类型枚举
     *
     * 用于描述解析后的表达式项的语义类别。
     */
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

    /**
     * @brief 骰子表达式解析后的词法单元结构体
     *
     * 该结构体存储了单个表达式项（如 `2d6` 或 `+5`）的完整信息，
     * 包括类型、正负号、骰子数量、面数、最小面值以及常数值。
     *
     * @note 常数值项中 `count` 和 `faces` 字段无效，骰子项中 `constant` 字段无效。
     */
    struct ExpressionTerm
    {
        TermType type;
        int sign;
        int count = 1;
        int faces = -1;
        int faces_min = 1;
        int constant = 0;
    };
    

    /**
     * @brief 骰子投掷命令处理器
     *
     * `RollCommand` 实现了 `.r` / `.roll` 等命令的处理逻辑，负责解析用户输入的
     * 骰子表达式（如 `2d6+1d4-3`），调用当前上下文关联的 `IDiceSystem` 执行投掷，
     * 并将详细结果返回给用户。
     *
     * 命令执行流程：
     * 1. 从上下文中获取关联的骰子系统（通过 `IDiceContextProvider`）。
     * 2. 解析命令参数字符串为 `ExpressionTerm` 列表。
     * 3. 将解析结果转换为骰子系统可接受的配置，执行投掷。
     * 4. 格式化投掷日志并通过 `context.SendResult()` 发送。
     *
     * @see Command_Core::ICommandHandler, IDiceSystem, IDiceContextProvider
     */
    class RollCommand : public Command_Core::ICommandHandler
    {
    public:
        RollCommand() = default;

        /**
         * @brief 获取命令的正式名称
         * @return 字符串 "roll"
         */
        std::string_view GetName() const override
        {
            return "roll";
        }

        /**
         * @brief 获取命令的功能描述
         * @return 描述字符串
         */
        std::string_view GetDescription() const override
        {
            return "骰子命令\"roll\"。支持表达式如：!r 2d6+1d4-1d4-3+3等。";
        }

        /**
         * @brief 获取命令的别名列表
         *
         * 支持大小写变体及常见缩写，如 `r`、`rd`、`dice`、`throw` 等。
         *
         * @return 别名视图向量
         */
        std::vector<std::string_view> GetAlias() const override
        {
            return {"r", "rd", "dice", "throw", "roll", "R", "RD", "DICE", "THROW", "ROLL"};
        }

        /**
         * @brief 执行命令
         *
         * @param context 命令执行上下文，提供参数、用户信息及结果发送能力
         */
        void Execute(Command_Core::ICommandContext& context) override;

    private:

        
        /**
         * @brief 解析骰子表达式字符串为词法单元列表
         *
         * 支持格式如 `2d6+1d4-3+2d20`，能够处理正负号和连续骰子项。
         *
         * @param expr 待解析的表达式字符串视图
         * @return 解析后的 `ExpressionTerm` 向量
         */
        static std::vector<ExpressionTerm> ParseDiceExpression(std::string_view expr);


        /**
         * @brief 执行实际投掷并生成结果字符串
         *
         * @param term 解析后的表达式项列表
         * @param dice_system 指向当前会话骰子系统的指针
         * @param userID 投掷者用户 ID
         * @return 格式化后的投掷结果日志字符串
         */
        std::string ExecuteDiceRoll(const std::vector<ExpressionTerm>& term, SaiKoLily::DiceSystem::IDiceSystem* dice_system, uint64 userID);

        ///< 缓存的骰子系统指针，在命令执行过程中使用，避免多次从上下文获取
        SaiKoLily::DiceSystem::IDiceSystem* dice_system_;
    };

    /**
     * @brief 将骰子相关命令注册到命令注册表中
     *
     * 该函数创建 `RollCommand` 和 `HistoryCommand` 实例并注册到给定的注册器。
     *
     * @param registry 命令注册器接口引用
     *
     * @see Command_Core::ICommandRegistry, RollCommand, HistoryCommand
     */
    void RegisterDiceCommands(Command_Core::ICommandRegistry& registry);
}


/// @warning 此模块可能未被使用 @author Sora32314 @date 2026-04-22
namespace SaiKoLily::CheckCommand
{
    /**
     * @brief 设置历史查询命令模块的日志回调
     *
     * @param callback 日志回调函数
     */
    void SetLogCallback(SaiKo_LogCallback callback);

    /**
     * @brief 历史查询命令的子命令类型枚举
     *
     * 定义 `.checkhistory` 命令支持的各类操作。
     */
    enum class TermType
    {
        List = 1,   ///< 列出历史记录
        Last,       ///< 查看最近一次投掷
        Name,       ///< 按玩家名查询
        ID,         ///< 按用户 ID 查询
        Detail,     ///< 查看某次投掷详情
        Stats,      ///< 统计信息
        Clear,      ///< 清空历史
        Check,      ///< 通用查询
        NULLCommand ///< 未识别命令
    };

    /**
     * @brief 历史查询命令解析后的表达式项
     *
     * 包含子命令类型及其参数列表。
     */
    struct ExpressionTerm
    {
        using Args = std::string;
        TermType type = TermType::NULLCommand;
        std::vector<Args> args;
    };


    /**
     * @brief 历史记录查询命令处理器
     *
     * `HistoryCommand` 实现了 `.ch` / `.checkhistory` 等命令，用于查询当前会话中
     * 骰子投掷的历史记录。支持按数量列出、按玩家筛选、查看详情等子功能。
     *
     * 命令格式示例：
     * - `.ch list 10`  列出最近 10 条记录
     * - `.ch last`      查看最后一次投掷
     * - `.ch name 玩家名` 查询指定玩家的历史
     *
     * @see Command_Core::ICommandHandler, IDiceSystem
     */
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
        

        /**
         * @brief 执行历史查询命令
         *
         * @param context 命令执行上下文
         */
        void Execute(Command_Core::ICommandContext& context) override;

    private:
        /**
         * @brief 从骰子系统中获取指定数量的历史记录文本
         *
         * @param dice_system 骰子系统指针
         * @param userID 用户 ID（用于筛选，0 表示全部）
         * @param limit 获取条数上限
         * @return 历史记录字符串向量
         */
        std::vector<std::string> GetHistory(SaiKoLily::DiceSystem::IDiceSystem* dice_system,
                                            ID userID,
                                            size_t limit = 10);

        /**
         * @brief 将历史记录字符串列表格式化为单一输出字符串
         *
         * @param history 历史记录字符串向量
         * @param limit 显示条数限制
         * @return 格式化后的字符串
         */
        std::string GetHistoryString(const std::vector<std::string>& history, size_t limit = 10);

        /**
         * @brief 根据解析后的表达式项执行历史查询
         *
         * @param dice_system 骰子系统指针
         * @param context 命令上下文
         * @param terms 解析后的表达式项列表
         * @return 执行结果字符串
         */
        std::string ExecuteHistory(SaiKoLily::DiceSystem::IDiceSystem* dice_system,
                                   const Command_Core::ICommandContext& context,
                                   const std::vector<ExpressionTerm>& terms);

        /**
         * @brief 解析命令参数字符串为表达式项列表
         *
         * @param exprs 参数字符串视图向量
         * @param context 命令上下文（用于日志）
         * @return 解析后的 `ExpressionTerm` 向量
         */
        std::vector<ExpressionTerm> ParseHistoryExpression(std::vector<std::string_view> exprs,
                                                           Command_Core::ICommandContext& context);
    };
    
    /**
     * @brief 将历史查询命令注册到命令注册表中
     *
     * @param registry 命令注册器接口引用
     */
    void RegisterHistoryCommands(Command_Core::ICommandRegistry& registry);

    /**
     * @brief 参数到子命令类型的映射表
     *
     * 用于将用户输入的子命令字符串（如 "list"、"last"）映射到 `TermType` 枚举值。
     */
    extern const std::unordered_map<std::string_view, TermType> paramTypeMap;


}




