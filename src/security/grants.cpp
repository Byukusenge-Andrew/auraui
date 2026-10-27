#include "grants.hpp"
#include <random>
#include <sstream>
#include <iomanip>

namespace auraui {
namespace security {

std::string GrantTable::generate_random_128bit_id() {
    std::random_device rd;
    std::uniform_int_distribution<uint64_t> dist;
    uint64_t part1 = dist(rd);
    uint64_t part2 = dist(rd);

    std::ostringstream oss;
    oss << "grant_"
        << std::hex << std::setfill('0') << std::setw(16) << part1
        << std::setfill('0') << std::setw(16) << part2;
    return oss.str();
}

std::string GrantTable::create_grant(std::filesystem::path canonical_path, GrantAccess access) {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::string id = generate_random_128bit_id();
    GrantInfo info{
        id,
        std::move(canonical_path),
        access,
        std::chrono::steady_clock::now()
    };
    m_grants[id] = std::move(info);
    return id;
}

Result<std::filesystem::path> GrantTable::resolve_grant(const std::string& grant_id, GrantAccess required_access) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto it = m_grants.find(grant_id);
    if (it == m_grants.end()) {
        return make_error(ErrorCode::not_found, "Invalid or expired grant ID");
    }

    const GrantInfo& info = it->second;

    if (required_access == GrantAccess::write && info.access == GrantAccess::read) {
        return make_error(ErrorCode::permission_denied, "Grant ID was issued for read-only access");
    }

    return info.path;
}

void GrantTable::revoke_grant(const std::string& grant_id) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_grants.erase(grant_id);
}

void GrantTable::clear() {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_grants.clear();
}

} // namespace security
} // namespace auraui
