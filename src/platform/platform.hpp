#pragma once

#include "auraui/result.hpp"
#include "auraui/error.hpp"
#include "nlohmann/json.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <filesystem>
#include <optional>
#include <cstdint>

namespace auraui {
namespace platform {

struct FileDialogFilter {
    std::string name;
    std::vector<std::string> extensions; // e.g. {"png", "jpg"}
};

struct FileDialogOptions {
    std::string title;
    std::string default_path;
    std::vector<FileDialogFilter> filters;
    bool allow_multiple{false};
    bool choose_directories{false};
};

struct SystemInfo {
    std::string os_name;
    std::string os_version;
    std::string architecture;
    uint32_t cpu_count{0};
    uint64_t total_memory_bytes{0};
    uint64_t available_memory_bytes{0};
};

class Platform {
public:
    // File & folder dialogs
    [[nodiscard]] static Result<std::vector<std::string>> show_open_dialog(const FileDialogOptions& options, void* parent_window = nullptr);
    [[nodiscard]] static Result<std::string> show_save_dialog(const FileDialogOptions& options, void* parent_window = nullptr);

    // Clipboard
    [[nodiscard]] static Result<std::string> clipboard_read_text();
    [[nodiscard]] static Result<void> clipboard_write_text(std::string_view text);

    // Shell
    [[nodiscard]] static Result<void> shell_open_external(std::string_view uri);

    // System info
    [[nodiscard]] static SystemInfo get_system_info();

    // Standard directories
    [[nodiscard]] static std::filesystem::path app_data_dir(const std::string& app_id = "");
    [[nodiscard]] static std::filesystem::path cache_dir(const std::string& app_id = "");
    [[nodiscard]] static std::filesystem::path temp_dir();
    [[nodiscard]] static std::filesystem::path home_dir();
};

} // namespace platform
} // namespace auraui
