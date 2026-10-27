#include "capability_policy.hpp"

namespace auraui {
namespace security {

CapabilityPolicy::CapabilityPolicy(const nlohmann::json& manifest_capabilities) {
    if (!manifest_capabilities.is_object()) {
        return;
    }

    for (auto it = manifest_capabilities.begin(); it != manifest_capabilities.end(); ++it) {
        const std::string& cap_name = it.key();
        const auto& val = it.value();

        if (val.is_boolean() && val.get<bool>()) {
            m_granted_capabilities.insert(cap_name);
        } else if (val.is_object()) {
            m_granted_capabilities.insert(cap_name);
            m_capability_options[cap_name] = val;
        }
    }
}

bool CapabilityPolicy::has_capability(const std::string& capability) const noexcept {
    return m_granted_capabilities.find(capability) != m_granted_capabilities.end();
}

const nlohmann::json* CapabilityPolicy::get_options(const std::string& capability) const noexcept {
    auto it = m_capability_options.find(capability);
    if (it != m_capability_options.end()) {
        return &(it->second);
    }
    return nullptr;
}

} // namespace security
} // namespace auraui
