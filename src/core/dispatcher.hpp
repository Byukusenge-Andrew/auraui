#pragma once

#include "auraui/command.hpp"
#include "auraui/middleware.hpp"
#include "auraui/router.hpp"
#include "auraui/result.hpp"
#include "ipc_envelope.hpp"
#include "rate_limiter.hpp"
#include "thread_pool.hpp"
#include "security/capability_policy.hpp"

#include <string>
#include <string_view>
#include <memory>
#include <unordered_map>
#include <vector>
#include <mutex>

namespace auraui {
namespace core {

class Dispatcher {
public:
    struct RegisteredCommand {
        CommandSpec spec;
        CommandHandler handler;
        std::vector<Middleware> middlewares;
    };

private:
    std::mutex m_mutex;
    std::unordered_map<std::string, RegisteredCommand> m_commands;
    std::vector<Middleware> m_global_middlewares;
    security::CapabilityPolicy m_policy;
    RateLimiter m_rate_limiter;
    std::shared_ptr<ThreadPool> m_pool;

public:
    Dispatcher(security::CapabilityPolicy policy, uint32_t rate_limit_per_sec = 100, std::shared_ptr<ThreadPool> pool = nullptr);

    Result<void> register_command(CommandSpec spec, CommandHandler handler, std::vector<Middleware> mws = {});
    void use(Middleware mw);
    void mount(const std::string& prefix, const Router& router);

    void dispatch(std::string_view raw_request_json, std::function<void(std::string)> reply_callback);

    [[nodiscard]] const security::CapabilityPolicy& policy() const noexcept { return m_policy; }
};

} // namespace core
} // namespace auraui
