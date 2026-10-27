#pragma once

#include "auraui/result.hpp"
#include <string>
#include <filesystem>
#include <mutex>
#include <unordered_map>
#include <chrono>

namespace auraui {
namespace security {

enum class GrantAccess {
    read,
    write,
    read_write
};

struct GrantInfo {
    std::string id;
    std::filesystem::path path;
    GrantAccess access{GrantAccess::read};
    std::chrono::steady_clock::time_point created_at;
};

class GrantTable {
private:
    std::mutex m_mutex;
    std::unordered_map<std::string, GrantInfo> m_grants;

    static std::string generate_random_128bit_id();

public:
    GrantTable() = default;

    std::string create_grant(std::filesystem::path canonical_path, GrantAccess access);
    [[nodiscard]] Result<std::filesystem::path> resolve_grant(const std::string& grant_id, GrantAccess required_access);
    void revoke_grant(const std::string& grant_id);
    void clear();
};

} // namespace security
} // namespace auraui
