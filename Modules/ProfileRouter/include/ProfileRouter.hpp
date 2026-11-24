#pragma once

#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <string_view>

#include <CommandCore.hpp>
#include <loggings.hpp>


namespace ProfileRouter
{
    using PR_LogCallback = std::function<void(const std::string&, Plugin_Logs::logLevel, bool nowFlush)>;
    void SetLogCallback(PR_LogCallback callback);

    /**
     * @brief 规则包中的单个命令定义（只读）
     *
     * 当前只暴露路由所需的最小信息；后续 action / resolution / output
     * 等字段由 ProfileParser 与 ResolutionEngine 扩展。
     */
    class IProfileCommand
    {
    public:
        virtual ~IProfileCommand() = default;

        virtual std::string_view GetName() const = 0;

        virtual Command_Core::UserLevel GetRequiredLevel() const = 0;
    };

    /**
     * @brief 规则包只读视图
     *
     * 规则包是会话级绑定的不可变数据；它**不是**命令处理器。
     * 动态命令统一由 ProfileCommandRouter 作为 fallback 路由。
     */
    class IProfile
    {
    public:
        virtual ~IProfile() = default;

        virtual std::string_view GetId() const = 0;
        virtual std::string_view GetName() const = 0;
        virtual std::string_view GetVersion() const = 0;

        /**
         * @brief 按命令名查找命令定义
         *
         * @param command 用户原样输入的命令 token（不含前缀；别名不会自动归一化，
         *                profile 实现需要自行处理别名与大小写）
         * @return 借用指针，生命周期由 profile 保证；未定义时返回 nullptr。
         *         调用方应同时持有 profile 的 shared_ptr。
         */
        virtual const IProfileCommand* FindCommand(std::string_view command) const = 0;
    };

    /**
     * @brief 一次解析的结果快照
     *
     * `command` 指向 `profile` 内部的数据，因此只要持有 `profile`，
     * `command` 就保持有效。
     */
    struct ProfileResolution
    {
        std::shared_ptr<const IProfile> profile;
        const IProfileCommand* command = nullptr;
        Command_Core::UserLevel userLevel = Command_Core::UserLevel::NonMember;
    };

    /**
     * @brief 会话侧能力：按命令上下文解析当前会话的规则包与权限
     *
     * 由 SaiKoLily 实现并在 Plugin 初始化时注入。实现必须在
     * SessionManager 的锁内完成解析，不得把 Session* 暴露到锁外。
     */
    class IProfileSessionProvider
    {
    public:
        virtual ~IProfileSessionProvider() = default;

        /**
         * @brief 一次解析：当前会话 → profile → 命令 → 权限
         *
         * @return 任一环节缺失（未选会话 / 未注入规则包 / 未定义命令）时返回 nullopt
         */
        virtual std::optional<ProfileResolution> Resolve(
            const Command_Core::ICommandContext& context) const = 0;
    };

    /**
     * @brief 规则包动态命令的 fallback 路由
     *
     * 契约：
     * - `GetName()` 固定返回 "profile_router"；
     * - `CanHandle()` 仅判断 provider 是否已接线；
     * - `TryHandle()` 返回 false 表示“不适用”，checker 会继续尝试其他 fallback；
     *   权限不足时返回 true，并向用户发送明确提示。
     */
    class IProfileCommandRouter : public Command_Core::IFallbackHandler
    {
    public:
        ~IProfileCommandRouter() override = default;
    };

    /**
     * @brief 创建规则包命令路由器
     *
     * @param sessionProvider 会话侧 provider；允许为 nullptr（此时路由只返回 false）
     * @return 所有权交给调用方，通常直接传给 ICommandChecker::AddFallbackHandler()
     */
    std::unique_ptr<IProfileCommandRouter> CreateCommandRouter(
        std::shared_ptr<IProfileSessionProvider> sessionProvider);
}
