#include "config.hpp"
#include "utf8.hpp"

#include <fstream>
#include <unordered_set>

namespace auraui {
namespace core {

Result<AppConfig> ConfigLoader::load_from_file(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        return make_error(ErrorCode::not_found, "Manifest file not found: " + path);
    }

    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    return parse_and_validate(content);
}

Result<AppConfig> ConfigLoader::parse_and_validate(std::string_view json_content) {
    if (!Utf8::is_valid(json_content)) {
        return make_error(ErrorCode::invalid_argument, "Manifest file contains invalid UTF-8 bytes");
    }

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(json_content);
    } catch (const std::exception& ex) {
        return make_error(ErrorCode::invalid_argument, std::string("Malformed JSON manifest: ") + ex.what());
    }

    if (!j.is_object()) {
        return make_error(ErrorCode::invalid_argument, "Manifest root must be a JSON object");
    }

    // Top-level allowed keys
    static const std::unordered_set<std::string> allowed_top_keys = {
        "$schema", "version", "app", "window", "capabilities", "ipc", "dev"
    };

    for (auto it = j.begin(); it != j.end(); ++it) {
        if (allowed_top_keys.find(it.key()) == allowed_top_keys.end()) {
            return make_error(ErrorCode::invalid_argument, "Unknown top-level manifest key: " + it.key());
        }
    }

    // Check version
    if (!j.contains("version") || !j["version"].is_number_integer()) {
        return make_error(ErrorCode::invalid_argument, "Manifest missing required integer 'version'");
    }

    int ver = j["version"].get<int>();
    if (ver != 1) {
        return make_error(ErrorCode::unsupported, "Unsupported manifest version: " + std::to_string(ver));
    }

    // Check app section
    if (!j.contains("app") || !j["app"].is_object()) {
        return make_error(ErrorCode::invalid_argument, "Manifest missing required object 'app'");
    }

    const auto& app = j["app"];
    static const std::unordered_set<std::string> allowed_app_keys = {"id", "name", "version"};
    for (auto it = app.begin(); it != app.end(); ++it) {
        if (allowed_app_keys.find(it.key()) == allowed_app_keys.end()) {
            return make_error(ErrorCode::invalid_argument, "Unknown key in 'app' manifest section: " + it.key());
        }
    }

    if (!app.contains("id") || !app["id"].is_string() || app["id"].get<std::string>().empty()) {
        return make_error(ErrorCode::invalid_argument, "Manifest 'app' requires non-empty string 'id'");
    }

    if (!app.contains("name") || !app["name"].is_string() || app["name"].get<std::string>().empty()) {
        return make_error(ErrorCode::invalid_argument, "Manifest 'app' requires non-empty string 'name'");
    }

    AppConfig config;
    config.version = ver;
    config.app_id = app["id"].get<std::string>();
    config.app_name = app["name"].get<std::string>();
    if (app.contains("version") && app["version"].is_string()) {
        config.app_version = app["version"].get<std::string>();
    }

    // Window section
    if (j.contains("window")) {
        const auto& win = j["window"];
        if (!win.is_object()) {
            return make_error(ErrorCode::invalid_argument, "'window' section must be an object");
        }
        static const std::unordered_set<std::string> allowed_win_keys = {
            "title", "width", "height", "min_width", "min_height", "frameless", "resizable"
        };
        for (auto it = win.begin(); it != win.end(); ++it) {
            if (allowed_win_keys.find(it.key()) == allowed_win_keys.end()) {
                return make_error(ErrorCode::invalid_argument, "Unknown key in 'window' manifest section: " + it.key());
            }
        }
        if (win.contains("title") && win["title"].is_string()) config.window_title = win["title"].get<std::string>();
        if (win.contains("width") && win["width"].is_number_unsigned()) config.window_width = win["width"].get<uint32_t>();
        if (win.contains("height") && win["height"].is_number_unsigned()) config.window_height = win["height"].get<uint32_t>();
        if (win.contains("min_width") && win["min_width"].is_number_unsigned()) config.window_min_width = win["min_width"].get<uint32_t>();
        if (win.contains("min_height") && win["min_height"].is_number_unsigned()) config.window_min_height = win["min_height"].get<uint32_t>();
        if (win.contains("frameless") && win["frameless"].is_boolean()) config.window_frameless = win["frameless"].get<bool>();
        if (win.contains("resizable") && win["resizable"].is_boolean()) config.window_resizable = win["resizable"].get<bool>();
    } else {
        config.window_title = config.app_name;
    }

    // Capabilities section
    if (j.contains("capabilities")) {
        if (!j["capabilities"].is_object()) {
            return make_error(ErrorCode::invalid_argument, "'capabilities' section must be an object");
        }
        config.capabilities = j["capabilities"];
    }

    // IPC section
    if (j.contains("ipc")) {
        const auto& ipc = j["ipc"];
        if (!ipc.is_object()) {
            return make_error(ErrorCode::invalid_argument, "'ipc' section must be an object");
        }
        if (ipc.contains("max_request_bytes") && ipc["max_request_bytes"].is_number_unsigned()) {
            config.ipc_max_request_bytes = ipc["max_request_bytes"].get<size_t>();
        }
        if (ipc.contains("rate_limit_per_sec") && ipc["rate_limit_per_sec"].is_number_unsigned()) {
            config.ipc_rate_limit_per_sec = ipc["rate_limit_per_sec"].get<uint32_t>();
        }
    }

    // Dev section
    if (j.contains("dev")) {
#if !defined(AURA_DEV)
        // In release builds, assert/fail if dev options are set or ignore
#else
        const auto& dev = j["dev"];
        if (dev.is_object()) {
            if (dev.contains("devtools") && dev["devtools"].is_boolean()) config.dev_devtools = dev["devtools"].get<bool>();
            if (dev.contains("hot_reload") && dev["hot_reload"].is_boolean()) config.dev_hot_reload = dev["hot_reload"].get<bool>();
        }
#endif
    }

    return config;
}

} // namespace core
} // namespace auraui
