#include "store.hpp"
#include "utf8.hpp"

#include <fstream>
#include <cstdlib>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <shlobj.h>
#endif

namespace auraui {
namespace core {

static std::filesystem::path get_user_data_dir(const std::string& app_id) {
    namespace fs = std::filesystem;
#if defined(_WIN32)
    char appdata[MAX_PATH];
    if (SUCCEEDED(SHGetFolderPathA(NULL, CSIDL_APPDATA, NULL, 0, appdata))) {
        fs::path p = fs::path(appdata) / "AuraUI" / (app_id.empty() ? "default" : app_id);
        std::error_code ec;
        fs::create_directories(p, ec);
        return p / "store.json";
    }
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    if (home) {
        fs::path p = fs::path(home) / "Library" / "Application Support" / "AuraUI" / (app_id.empty() ? "default" : app_id);
        std::error_code ec;
        fs::create_directories(p, ec);
        return p / "store.json";
    }
#else
    const char* xdg = std::getenv("XDG_DATA_HOME");
    fs::path base;
    if (xdg && *xdg) {
        base = fs::path(xdg);
    } else {
        const char* home = std::getenv("HOME");
        base = home ? (fs::path(home) / ".local" / "share") : fs::path(".");
    }
    fs::path p = base / "auraui" / (app_id.empty() ? "default" : app_id);
    std::error_code ec;
    fs::create_directories(p, ec);
    return p / "store.json";
#endif
    return fs::path("store.json");
}

Store::Store(std::string app_id)
    : m_file_path(get_user_data_dir(app_id)) {}

Result<nlohmann::json> Store::read_file() const {
    std::error_code ec;
    if (!std::filesystem::exists(m_file_path, ec)) {
        return nlohmann::json::object();
    }

    auto size = std::filesystem::file_size(m_file_path, ec);
    if (!ec && size > MAX_STORE_SIZE_BYTES) {
        return make_error(ErrorCode::internal, "Store file size exceeds maximum 10 MiB limit");
    }

    std::ifstream file(m_file_path);
    if (!file.is_open()) {
        return make_error(ErrorCode::internal, "Failed to open store file for reading");
    }

    try {
        nlohmann::json root;
        file >> root;
        if (root.is_object() && root.contains("data") && root["data"].is_object()) {
            return root["data"];
        }
        if (root.is_object()) {
            return root;
        }
    } catch (...) {}
    return nlohmann::json::object();
}

Result<void> Store::write_file_atomic(const nlohmann::json& data) {
    std::string serialized = nlohmann::json{
        {"version", 1},
        {"data", data}
    }.dump(2, ' ', false, nlohmann::json::error_handler_t::replace);

    if (serialized.size() > MAX_STORE_SIZE_BYTES) {
        return make_error(ErrorCode::invalid_argument, "Store payload exceeds 10 MiB limit");
    }

    std::filesystem::path tmp_path = m_file_path;
    tmp_path += ".tmp";

    {
        std::ofstream out(tmp_path, std::ios::binary | std::ios::trunc);
        if (!out.is_open()) {
            return make_error(ErrorCode::internal, "Failed to create temporary store file");
        }
        out.write(serialized.data(), static_cast<std::streamsize>(serialized.size()));
        out.flush();
    }

    std::error_code ec;
    std::filesystem::rename(tmp_path, m_file_path, ec);
    if (ec) {
        // Fallback for Windows replacing existing file
#if defined(_WIN32)
        std::wstring w_tmp = Utf8::to_wide(tmp_path.string());
        std::wstring w_dest = Utf8::to_wide(m_file_path.string());
        if (!MoveFileExW(w_tmp.c_str(), w_dest.c_str(), MOVEFILE_REPLACE_EXISTING)) {
            return make_error(ErrorCode::internal, "Failed to replace store file atomically");
        }
#else
        return make_error(ErrorCode::internal, "Failed to rename temporary store file: " + ec.message());
#endif
    }

    return {};
}

Result<nlohmann::json> Store::get(const std::string& key) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto data_res = read_file();
    if (!data_res) return data_res;

    const auto& data = *data_res;
    if (data.contains(key)) {
        return data[key];
    }
    return make_error(ErrorCode::not_found, "Key '" + key + "' not found in store");
}

Result<nlohmann::json> Store::get_all() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return read_file();
}

Result<void> Store::set(const std::string& key, nlohmann::json value) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto data_res = read_file();
    nlohmann::json data = data_res ? *data_res : nlohmann::json::object();

    data[key] = std::move(value);
    return write_file_atomic(data);
}

Result<void> Store::remove(const std::string& key) {
    std::lock_guard<std::mutex> lock(m_mutex);
    auto data_res = read_file();
    if (!data_res) return {};

    nlohmann::json data = *data_res;
    if (data.contains(key)) {
        data.erase(key);
        return write_file_atomic(data);
    }
    return {};
}

} // namespace core
} // namespace auraui
