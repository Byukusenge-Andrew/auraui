#include "rate_limiter.hpp"

namespace auraui {
namespace core {

Result<void> RateLimiter::check(const std::string& command_name) {
    std::lock_guard<std::mutex> lock(m_mutex);

    // 1. Check global rate limit
    if (!m_global_bucket.try_consume(1.0)) {
        return make_error(ErrorCode::rate_limited, "Global request rate limit exceeded");
    }

    // 2. Check per-command rate limit (default 50 req/sec per command)
    auto it = m_command_buckets.find(command_name);
    if (it == m_command_buckets.end()) {
        it = m_command_buckets.emplace(command_name, TokenBucket(50.0, 50.0)).first;
    }

    if (!it->second.try_consume(1.0)) {
        return make_error(ErrorCode::rate_limited, "Command '" + command_name + "' rate limit exceeded");
    }

    return {};
}

} // namespace core
} // namespace auraui
