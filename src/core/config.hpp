#pragma once

#include "auraui/result.hpp"
#include "auraui/error.hpp"
#include "nlohmann/json.hpp"

#include <string>
#include <string_view>
#include <cstdint>

namespace auraui {
namespace core {

struct AppConfig {
    int version{1};
    std::string app_id;
    std::string app_name;
    std::string app_version{"1.0.0"};

    std::string window_title{"AuraUI Application"};
    uint32_t window_width{1000};
    uint32_t window_height{700};
    uint32_t window_min_width{0};
    uint32_t window_min_height{0};
    bool window_frameless{false};
    bool window_resizable{true};

    nlohmann::json capabilities = nlohmann::json::object();

    size_t ipc_max_request_bytes{1048576};
    uint32_t ipc_rate_limit_per_sec{100};

    bool dev_devtools{false};
    bool dev_hot_reload{false};
};

class ConfigLoader {
public:
    [[nodiscard]] static Result<AppConfig> load_from_file(const std::string& path);
    [[nodiscard]] static Result<AppConfig> parse_and_validate(std::string_view json_content);
};

} // namespace core
} // namespace auraui
