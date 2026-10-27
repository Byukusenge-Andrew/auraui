#pragma once

#include "auraui/result.hpp"
#include <string>
#include <mutex>
#include <unordered_map>
#include <chrono>

namespace auraui {
namespace core {

struct TokenBucket {
    double capacity{100.0};
    double refill_rate{100.0}; // tokens per second
    double tokens{100.0};
    std::chrono::steady_clock::time_point last_refill;

    TokenBucket() : last_refill(std::chrono::steady_clock::now()) {}

    TokenBucket(double cap, double rate)
        : capacity(cap), refill_rate(rate), tokens(cap), last_refill(std::chrono::steady_clock::now()) {}

    bool try_consume(double count = 1.0) {
        auto now = std::chrono::steady_clock::now();
        double elapsed_sec = std::chrono::duration<double>(now - last_refill).count();
        last_refill = now;

        tokens = std::min(capacity, tokens + (elapsed_sec * refill_rate));
        if (tokens >= count) {
            tokens -= count;
            return true;
        }
        return false;
    }
};

class RateLimiter {
private:
    std::mutex m_mutex;
    TokenBucket m_global_bucket;
    std::unordered_map<std::string, TokenBucket> m_command_buckets;

public:
    explicit RateLimiter(uint32_t global_limit_per_sec = 100)
        : m_global_bucket(static_cast<double>(global_limit_per_sec), static_cast<double>(global_limit_per_sec)) {}

    [[nodiscard]] Result<void> check(const std::string& command_name);
};

} // namespace core
} // namespace auraui
