#pragma once

#include "auraui/capabilities.hpp"
#include "auraui/result.hpp"
#include "nlohmann/json.hpp"

#include <string>
#include <unordered_set>
#include <unordered_map>

namespace auraui {
namespace security {

class CapabilityPolicy {
private:
    std::unordered_set<std::string> m_granted_capabilities;
    std::unordered_map<std::string, nlohmann::json> m_capability_options;

public:
    CapabilityPolicy() = default;

    explicit CapabilityPolicy(const nlohmann::json& manifest_capabilities);

    [[nodiscard]] bool has_capability(const std::string& capability) const noexcept;
    [[nodiscard]] const nlohmann::json* get_options(const std::string& capability) const noexcept;
    [[nodiscard]] const std::unordered_set<std::string>& granted() const noexcept { return m_granted_capabilities; }
};

} // namespace security
} // namespace auraui
