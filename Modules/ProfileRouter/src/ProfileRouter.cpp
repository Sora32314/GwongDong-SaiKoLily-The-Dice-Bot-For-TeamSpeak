#include <ProfileRouter.hpp>

#include <format>
#include <utility>


namespace ProfileRouter
{
    static PR_LogCallback pr_logCallback = nullptr;

    void SetLogCallback(PR_LogCallback callback)
    {
        pr_logCallback = callback;
    }

    namespace
    {
        void Log(std::string_view message, Plugin_Logs::logLevel level, bool nowFlush = false)
        {
            if(pr_logCallback)
                pr_logCallback(std::string(message), level, nowFlush);
        }

        class ProfileCommandRouterImpl final : public IProfileCommandRouter
        {
        public:
            explicit ProfileCommandRouterImpl(std::shared_ptr<IProfileSessionProvider> provider) : sessionProvider(std::move(provider))
            {
                if(!sessionProvider)
                {
                    Log(std::format("ProfileRouter: 未提供 IProfileSessionProvider，profile 命令将不会被路由。"),
                        Plugin_Logs::logLevel::warn, false);
                }
            }

            std::string_view GetName() const override
            {
                return "profile_router";
            }

            bool CanHandle(const Command_Core::ICommandContext& context) const override
            {
                // 真正的适用性在 TryHandle 中通过 Resolve 判断。
                #ifdef _DEBUG
                    Log(std::format("ProfileRouter: 尝试Handle命令，调用者：{}。", context.GetCallingUserName()),
                        Plugin_Logs::logLevel::info, false);
                #endif

                return sessionProvider != nullptr;
            }

            bool TryHandle(const Command_Core::ICommandContext& context) override
            {
                if(!sessionProvider)
                    return false;

                auto resolution = sessionProvider->Resolve(context);
                if(!resolution || !resolution->profile || !resolution->command)
                    return false;

                if(resolution->userLevel > resolution->command->GetRequiredLevel())
                {
                    context.SendResult(std::format(
                        "权限不足：规则包 \"{}\" 的命令 \"{}\" 需要更高权限。",
                        std::string(resolution->profile->GetId()),
                        std::string(resolution->command->GetName())),
                        context.GetMessageTarget());
                    return true;
                }

                // TODO(Phase 6): 按 IProfileCommand 的 action 分发到
                // ResolutionEngine / CharacterSheet / SessionLog。
                context.SendResult(std::format(
                    "规则包 \"{}\"：命令 \"{}\" 已路由（action 待实现）。",
                    std::string(resolution->profile->GetId()),
                    std::string(resolution->command->GetName())),
                    context.GetMessageTarget());
                return true;
            }

        private:
            std::shared_ptr<IProfileSessionProvider> sessionProvider;
        };
    }

    std::unique_ptr<IProfileCommandRouter> CreateCommandRouter(
        std::shared_ptr<IProfileSessionProvider> sessionProvider)
    {
        return std::make_unique<ProfileCommandRouterImpl>(std::move(sessionProvider));
    }
}
