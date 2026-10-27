#pragma once

#include "nlohmann/json.hpp"

#include <string>
#include <fstream>
#include <filesystem>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#endif

namespace auraui {
namespace storage {

using json = nlohmann::json;

inline std::string get_storage_path(const std::string& app_name) {
#if defined(_WIN32)
    char appdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata))) {
        namespace fs = std::filesystem;
        fs::path p = fs::path(appdata) / "AuraUI" / (app_name.empty() ? "DefaultApp" : app_name);
        std::error_code ec;
        fs::create_directories(p, ec);
        return (p / "store.json").string();
    }
#endif
    return "store.json";
}

inline json load_storage(const std::string& app_name) {
    namespace fs = std::filesystem;
    std::string path = get_storage_path(app_name);
    std::error_code ec;
    if (!fs::exists(path, ec)) return json::object();
    try {
        std::ifstream f(path);
        if (f.is_open()) {
            json data;
            f >> data;
            if (data.is_object()) return data;
        }
    } catch (...) {}
    return json::object();
}

inline bool save_storage(const std::string& app_name, const json& data) {
    std::string path = get_storage_path(app_name);
    try {
        std::ofstream f(path);
        if (f.is_open()) {
            f << data.dump(2, ' ', false, json::error_handler_t::replace);
            return true;
        }
    } catch (...) {}
    return false;
}

} // namespace storage
} // namespace auraui
