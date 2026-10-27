#pragma once

#include <string>
#include <string_view>
#include <array>

namespace auraui {
namespace capabilities {

inline constexpr std::string_view WINDOW_CONTROL    = "window.control";
inline constexpr std::string_view DIALOG_OPEN       = "dialog.open";
inline constexpr std::string_view DIALOG_SAVE       = "dialog.save";
inline constexpr std::string_view DIALOG_FOLDER     = "dialog.folder";
inline constexpr std::string_view CLIPBOARD_WRITE   = "clipboard.write";
inline constexpr std::string_view CLIPBOARD_READ    = "clipboard.read";
inline constexpr std::string_view SHELL_OPEN_URL    = "shell.open_url";
inline constexpr std::string_view SHELL_REVEAL_PATH = "shell.reveal_path";
inline constexpr std::string_view FS_READ_GRANTED   = "fs.read_granted";
inline constexpr std::string_view FS_WRITE_GRANTED  = "fs.write_granted";
inline constexpr std::string_view SYSTEM_INFO       = "system.info";
inline constexpr std::string_view SYSTEM_IDENTITY   = "system.identity";
inline constexpr std::string_view STORE_LOCAL       = "store.local";

inline constexpr std::array<std::string_view, 13> ALL = {
    WINDOW_CONTROL,
    DIALOG_OPEN,
    DIALOG_SAVE,
    DIALOG_FOLDER,
    CLIPBOARD_WRITE,
    CLIPBOARD_READ,
    SHELL_OPEN_URL,
    SHELL_REVEAL_PATH,
    FS_READ_GRANTED,
    FS_WRITE_GRANTED,
    SYSTEM_INFO,
    SYSTEM_IDENTITY,
    STORE_LOCAL
};

[[nodiscard]] inline bool is_valid_capability(std::string_view name) noexcept {
    for (const auto& cap : ALL) {
        if (cap == name) return true;
    }
    return false;
}

} // namespace capabilities
} // namespace auraui
