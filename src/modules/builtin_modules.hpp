#pragma once

#include "auraui/app.hpp"
#include "auraui/router.hpp"

namespace auraui {
namespace modules {

Router create_window_router(App* app);
Router create_dialog_router(void* native_window_handle = nullptr);
Router create_clipboard_router();
Router create_system_router();
Router create_shell_router();
Router create_fs_router(std::filesystem::path app_data_dir);
Router create_store_router();

void register_all(App& app, void* native_window = nullptr);

} // namespace modules
} // namespace auraui
