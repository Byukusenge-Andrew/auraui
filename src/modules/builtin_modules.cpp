#include "builtin_modules.hpp"
#include "platform/platform.hpp"
#include "core/store.hpp"
#include "security/path_policy.hpp"
#include "security/url_policy.hpp"
#include "auraui/log.hpp"

#include <fstream>
#include <sstream>

namespace auraui {
namespace modules {

Router create_window_router(App* app) {
    Router r;

    r.command(CommandSpec("setTitle"), [app](Context ctx, Reply reply) {
        std::string title = ctx.get<std::string>("title", "");
        if (app && !title.empty()) {
            app->set_title(title);
            reply.send({{"success", true}});
        } else {
            reply.error(ErrorCode::invalid_argument, "Missing or empty title parameter");
        }
    });

    r.command(CommandSpec("setSize"), [app](Context ctx, Reply reply) {
        int width = ctx.get<int>("width", 0);
        int height = ctx.get<int>("height", 0);
        if (app && width > 0 && height > 0) {
            app->set_size(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
            reply.send({{"success", true}});
        } else {
            reply.error(ErrorCode::invalid_argument, "Invalid width or height parameters");
        }
    });

    r.command(CommandSpec("close"), [app](Context, Reply reply) {
        if (app) {
            app->close();
            reply.send({{"success", true}});
        } else {
            reply.error(ErrorCode::internal, "App instance not available");
        }
    });

    return r;
}

Router create_dialog_router(void* native_window_handle) {
    Router r;

    CommandSpec open_spec("openFile");
    open_spec.with_capability("dialog.open");
    r.command(open_spec, [native_window_handle](Context ctx, Reply reply) {
        platform::FileDialogOptions opts;
        opts.title = ctx.get<std::string>("title", "Open File");
        opts.allow_multiple = ctx.get<bool>("multiple", false);
        opts.choose_directories = ctx.get<bool>("directory", false);

        if (ctx.has("filters") && ctx.args()["filters"].is_array()) {
            for (const auto& item : ctx.args()["filters"]) {
                if (item.is_object() && item.contains("name") && item.contains("extensions")) {
                    platform::FileDialogFilter filter;
                    filter.name = item["name"].get<std::string>();
                    for (const auto& ext : item["extensions"]) {
                        if (ext.is_string()) filter.extensions.push_back(ext.get<std::string>());
                    }
                    opts.filters.push_back(std::move(filter));
                }
            }
        }

        auto res = platform::Platform::show_open_dialog(opts, native_window_handle);
        if (res) {
            reply.send({
                {"canceled", res->empty()},
                {"filePaths", *res}
            });
        } else {
            reply.error(res.error());
        }
    });

    CommandSpec save_spec("saveFile");
    save_spec.with_capability("dialog.save");
    r.command(save_spec, [native_window_handle](Context ctx, Reply reply) {
        platform::FileDialogOptions opts;
        opts.title = ctx.get<std::string>("title", "Save File");

        auto res = platform::Platform::show_save_dialog(opts, native_window_handle);
        if (res) {
            reply.send({
                {"canceled", res->empty()},
                {"filePath", *res}
            });
        } else {
            reply.error(res.error());
        }
    });

    return r;
}

Router create_clipboard_router() {
    Router r;

    CommandSpec read_spec("readText");
    read_spec.with_capability("clipboard.read");
    r.command(read_spec, [](Context, Reply reply) {
        auto res = platform::Platform::clipboard_read_text();
        if (res) {
            reply.send({{"text", *res}});
        } else {
            reply.error(res.error());
        }
    });

    CommandSpec write_spec("writeText");
    write_spec.with_capability("clipboard.write");
    r.command(write_spec, [](Context ctx, Reply reply) {
        std::string text = ctx.get<std::string>("text", "");
        auto res = platform::Platform::clipboard_write_text(text);
        if (res) {
            reply.send({{"success", true}});
        } else {
            reply.error(res.error());
        }
    });

    return r;
}

Router create_system_router() {
    Router r;

    CommandSpec info_spec("getInfo");
    info_spec.with_capability("system.info");
    r.command(info_spec, [](Context, Reply reply) {
        auto info = platform::Platform::get_system_info();
        reply.send({
            {"os", info.os_name},
            {"version", info.os_version},
            {"arch", info.architecture},
            {"cpuCount", info.cpu_count},
            {"totalMemoryBytes", info.total_memory_bytes},
            {"availableMemoryBytes", info.available_memory_bytes}
        });
    });

    return r;
}

Router create_shell_router() {
    Router r;

    CommandSpec open_spec("openExternal");
    open_spec.with_capability("shell.open_external");
    r.command(open_spec, [](Context ctx, Reply reply) {
        std::string url = ctx.get<std::string>("url", "");
        if (url.empty()) {
            reply.error(ErrorCode::invalid_argument, "Missing URL argument");
            return;
        }

        // Validate scheme (strictly allow only http and https)
        if (url.rfind("http://", 0) != 0 && url.rfind("https://", 0) != 0) {
            reply.error(ErrorCode::permission_denied, "Only HTTP and HTTPS URLs can be opened externally");
            return;
        }

        auto res = platform::Platform::shell_open_external(url);
        if (res) {
            reply.send({{"success", true}});
        } else {
            reply.error(res.error());
        }
    });

    return r;
}

Router create_fs_router(std::filesystem::path app_data_dir) {
    Router r;

    CommandSpec read_spec("readFile");
    read_spec.with_capability("fs.read");
    r.command(read_spec, [app_data_dir](Context ctx, Reply reply) {
        std::string path_str = ctx.get<std::string>("path", "");
        if (path_str.empty()) {
            reply.error(ErrorCode::invalid_argument, "Missing path argument");
            return;
        }

        auto safe = security::PathPolicy::validate_safe_path(path_str);
        if (!safe) {
            reply.error(safe.error());
            return;
        }
        std::filesystem::path full_path = std::filesystem::path(path_str);
        if (!full_path.is_absolute()) {
            full_path = app_data_dir / full_path;
        }
        auto canon = security::PathPolicy::canonicalize(full_path.string());
        if (!canon) {
            reply.error(canon.error());
            return;
        }
        auto check = security::PathPolicy::is_within_root(*canon, app_data_dir);
        if (!check) {
            reply.error(check.error());
            return;
        }

        std::ifstream file(*canon, std::ios::binary);
        if (!file.is_open()) {
            reply.error(ErrorCode::not_found, "File not found or cannot be opened: " + path_str);
            return;
        }

        std::string content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        reply.send({{"content", content}});
    });

    CommandSpec write_spec("writeFile");
    write_spec.with_capability("fs.write");
    r.command(write_spec, [app_data_dir](Context ctx, Reply reply) {
        std::string path_str = ctx.get<std::string>("path", "");
        std::string content = ctx.get<std::string>("content", "");
        if (path_str.empty()) {
            reply.error(ErrorCode::invalid_argument, "Missing path argument");
            return;
        }

        auto safe = security::PathPolicy::validate_safe_path(path_str);
        if (!safe) {
            reply.error(safe.error());
            return;
        }
        std::filesystem::path full_path = std::filesystem::path(path_str);
        if (!full_path.is_absolute()) {
            full_path = app_data_dir / full_path;
        }
        std::error_code ec;
        std::filesystem::create_directories(full_path.parent_path(), ec);
        auto canon = security::PathPolicy::canonicalize(full_path.string());
        if (!canon) {
            reply.error(canon.error());
            return;
        }
        auto check = security::PathPolicy::is_within_root(*canon, app_data_dir);
        if (!check) {
            reply.error(check.error());
            return;
        }

        std::ofstream file(*canon, std::ios::binary | std::ios::trunc);
        if (!file.is_open()) {
            reply.error(ErrorCode::permission_denied, "Cannot write to file: " + path_str);
            return;
        }

        file.write(content.data(), static_cast<std::streamsize>(content.size()));
        reply.send({{"bytesWritten", content.size()}});
    });

    return r;
}

Router create_store_router() {
    Router r;
    auto store = std::make_shared<core::Store>("auraui");

    CommandSpec get_spec("get");
    get_spec.with_capability("store.read");
    r.command(get_spec, [store](Context ctx, Reply reply) {
        std::string key = ctx.get<std::string>("key", "");
        auto val = store->get(key);
        if (val) {
            reply.send({{"found", true}, {"value", *val}});
        } else {
            reply.send({{"found", false}, {"value", nullptr}});
        }
    });

    CommandSpec set_spec("set");
    set_spec.with_capability("store.write");
    r.command(set_spec, [store](Context ctx, Reply reply) {
        std::string key = ctx.get<std::string>("key", "");
        if (key.empty()) {
            reply.error(ErrorCode::invalid_argument, "Missing key parameter");
            return;
        }
        nlohmann::json val = ctx["value"];
        auto set_res = store->set(key, val);
        if (!set_res) {
            reply.error(set_res.error());
            return;
        }
        reply.send({{"success", true}});
    });

    CommandSpec del_spec("delete");
    del_spec.with_capability("store.write");
    r.command(del_spec, [store](Context ctx, Reply reply) {
        std::string key = ctx.get<std::string>("key", "");
        auto rem_res = store->remove(key);
        reply.send({{"deleted", rem_res.has_value()}});
    });

    return r;
}

void register_all(App& app, void* native_window) {
    app.mount("window", create_window_router(&app));
    app.mount("dialog", create_dialog_router(native_window));
    app.mount("clipboard", create_clipboard_router());
    app.mount("system", create_system_router());
    app.mount("shell", create_shell_router());
    app.mount("fs", create_fs_router(platform::Platform::app_data_dir("auraui")));
    app.mount("store", create_store_router());
}

} // namespace modules
} // namespace auraui
