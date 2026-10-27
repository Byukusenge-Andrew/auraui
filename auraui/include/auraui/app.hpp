#pragma once

#include "unicode.hpp"
#include "system.hpp"
#include "storage.hpp"
#include "router.hpp"
#include "watcher.hpp"

#include <string>
#include <unordered_set>
#include <vector>
#include <iostream>
#include <algorithm>
#include <memory>
#include <stdexcept>

namespace auraui {

class App {
public:
    using JsonHandler = std::function<json(const Context&)>;
    using VoidHandler = std::function<void()>;

private:
    webui::window m_win;
    std::string m_title;
    bool m_is_frameless{false};
    HotReloader m_reloader;
    std::vector<Middleware> m_global_middlewares;
    bool m_clipboard_read_allowed{false}; // Security: Default-deny clipboard reading (opt-in)
    std::unordered_set<std::string> m_registered_routes;

    void register_route_name(const std::string& route) {
        if (m_registered_routes.find(route) != m_registered_routes.end()) {
            throw std::runtime_error("AuraUI Route Collision: Handler already registered for route '" + route + "'");
        }
        m_registered_routes.insert(route);
    }

    std::string get_storage_path() const {
        return storage::get_storage_path(m_title);
    }

    json load_storage() const {
        return storage::load_storage(m_title);
    }

    bool save_storage(const json& data) const {
        return storage::save_storage(m_title, data);
    }

public:
    App(const std::string& title = "AuraUI Application", unsigned int width = 1000, unsigned int height = 700)
        : m_title(title)
    {
        m_win.set_size(width, height);
        m_win.set_center();

        // Built-in window controls and services
        bind_internal_handlers();
    }

    ~App() {
        stop_hot_reload();
    }

    // Live Hot-Reload Engine for Development
    void enable_hot_reload(const std::string& watch_folder = "ui") {
        m_reloader.start(watch_folder, [this]() {
            m_win.run("location.reload();");
        });
    }

    void stop_hot_reload() {
        m_reloader.stop();
    }

    // Window configuration
    App& set_size(unsigned int width, unsigned int height) {
        m_win.set_size(width, height);
        return *this;
    }

    App& set_min_size(unsigned int width, unsigned int height) {
        m_win.set_minimum_size(width, height);
        return *this;
    }

    App& set_center() {
        m_win.set_center();
        return *this;
    }

    App& set_frameless(bool frameless = true) {
        m_is_frameless = frameless;
        m_win.set_frameless(frameless);
        return *this;
    }

    App& set_transparent(bool transparent = true) {
        m_win.set_transparent(transparent);
        return *this;
    }

    App& set_resizable(bool resizable = true) {
        m_win.set_resizable(resizable);
        return *this;
    }

    // Security policy: Control whether JavaScript frontend can read OS clipboard
    App& allow_clipboard_read(bool allow = true) {
        m_clipboard_read_allowed = allow;
        return *this;
    }

    // Expose a C++ JSON lambda callable from JS with global middleware support:
    // JS: const res = await aura.call("calc", { x: 5, y: 10 });
    App& bind(const std::string& name, JsonHandler handler) {
        register_route_name(name);
        m_win.bind(name, [this, handler, name](webui::window::event* e) {
            Req req(name, e);
            Res res;
            if (!m_global_middlewares.empty()) {
                auto exec = std::make_shared<PipelineExecution>(m_global_middlewares, [handler](Req& q, Res& s) {
                    Context ctx(q.get_event());
                    json result;
                    try {
                        result = handler(ctx);
                    } catch (const std::exception& ex) {
                        result = json{{"error", true}, {"message", ex.what()}};
                    } catch (...) {
                        result = json{{"error", true}, {"message", "Unknown exception"}};
                    }
                    s.json(result);
                });
                exec->next(req, res);
            } else {
                Context ctx(e);
                json result;
                try {
                    result = handler(ctx);
                } catch (const std::exception& ex) {
                    result = json{{"error", true}, {"message", ex.what()}};
                } catch (...) {
                    result = json{{"error", true}, {"message", "Unknown exception"}};
                }
                res.json(result);
            }
            try {
                e->return_string(res.to_json().dump(-1, ' ', false, json::error_handler_t::replace));
            } catch (...) {
                e->return_string("{\"error\":true}");
            }
        });
        return *this;
    }

    // Expose a simple void function
    App& bind_action(const std::string& name, VoidHandler handler) {
        register_route_name(name);
        m_win.bind(name, [handler](webui::window::event* e) {
            try {
                handler();
            } catch (...) {}
            e->return_bool(true);
        });
        return *this;
    }

    // Send an event to the frontend with safe JSON serialization
    void emit(const std::string& event_name, const json& payload) {
        try {
            std::string script = "if (window.aura && window.aura.__dispatch) { window.aura.__dispatch(" + 
                                 json(event_name).dump(-1, ' ', false, json::error_handler_t::replace) + ", " + 
                                 payload.dump(-1, ' ', false, json::error_handler_t::replace) + "); }";
            m_win.run(script);
        } catch (...) {}
    }

    // Execute arbitrary JavaScript code
    void eval_js(const std::string& script) {
        m_win.run(script);
    }

    // Programmatically navigate frontend view routes from C++
    void navigate(const std::string& route) {
        try {
            eval_js("if (window.aura && window.aura.router) { window.aura.router.navigate(" + 
                    json(route).dump(-1, ' ', false, json::error_handler_t::replace) + "); }");
        } catch (...) {}
    }

    // Set root directory for local web assets
    App& set_root_folder(const std::string& path) {
        m_win.set_root_folder(path);
        return *this;
    }

    // Set custom virtual file system handler for embedded single-exe distribution
    void set_vfs(const void* (*vfs_handler)(const char* filename, int* length)) {
        m_win.set_file_handler(vfs_handler);
    }

    // Serve HTML from directory or inline
    bool start(const std::string& entry_point = "index.html") {
#if defined(AURA_DEV_MODE)
        set_root_folder("ui");
        enable_hot_reload("ui");
#endif
        return m_win.show(entry_point);
    }

    // Start with Microsoft Edge or specific browser
    bool start_browser(const std::string& entry_point, unsigned int browser) {
        return m_win.show_browser(entry_point, browser);
    }

    // Start WebView2 direct window
    bool start_wv(const std::string& entry_point) {
        return m_win.show_wv(entry_point);
    }

    // Run the event loop until all windows are closed
    void run() {
        webui::wait();
        webui::clean();
    }

    // Exit application immediately
    void exit() {
        webui::exit();
    }

    webui::window& raw_window() { return m_win; }

    // ------------------------------------------------------------------------
    // Express-Style Application Routing & Middleware
    // ------------------------------------------------------------------------

    // Register a global middleware (runs on every route request)
    App& use(Middleware mw) {
        m_global_middlewares.push_back(std::move(mw));
        return *this;
    }

    // Mount a modular sub-router under a URL/prefix path
    App& use(const std::string& prefix, const Router& router) {
        mount_router_recursive(prefix, router, m_global_middlewares);
        return *this;
    }

    // Define a route handler: app.route("/path", [](Req& req, Res& res) { ... })
    App& route(const std::string& path, RouteHandler handler) {
        register_pipeline(path, m_global_middlewares, std::move(handler));
        return *this;
    }

    // Define a route handler returning direct JSON: app.route("/path", [](Req& req) -> json { ... })
    App& route(const std::string& path, SimpleHandler handler) {
        return route(path, [handler](Req& req, Res& res) {
            json out = handler(req);
            res.ok(out);
        });
    }

    // Define a route with route-specific middleware: app.route("/path", auth_guard, handler)
    App& route(const std::string& path, Middleware mw, RouteHandler handler) {
        std::vector<Middleware> mws = m_global_middlewares;
        mws.push_back(std::move(mw));
        register_pipeline(path, mws, std::move(handler));
        return *this;
    }

    // Aliases for route()
    App& on(const std::string& path, RouteHandler handler) {
        return route(path, std::move(handler));
    }

    App& on(const std::string& path, SimpleHandler handler) {
        return route(path, std::move(handler));
    }

    App& on(const std::string& path, Middleware mw, RouteHandler handler) {
        return route(path, std::move(mw), std::move(handler));
    }

    // ASCII developer experience banner
    void print_banner() const {
        std::cout << "\n\033[36m"
                  << "  ___                  _   _ ___ \n"
                  << " / _ \\                | | | |_ _|\n"
                  << "/ /_\\ \\_   _ _ __ __ _| | | || | \n"
                  << "|  _  | | | | '__/ _` | | | || | \n"
                  << "| | | | |_| | | | (_| | |_| || | \n"
                  << "\\_| |_/\\__,_|_|  \\__,_|\\___/|___|\n"
                  << " :: AuraUI Modern C++20 Desktop Engine ::\n\033[0m"
                  << std::endl;
    }

    // Check if any supported browser is installed on this machine
    bool has_browser() const {
        return m_win.get_best_browser() != 0;
    }

    // Fluent Express-style listen (prioritizes native in-process WebView2, falling back to installed browser)
    void listen(const std::string& entry_point = "index.html", std::function<void()> on_ready = nullptr) {
        print_banner();
        // 1. Primary path: Native in-process WebView2 window (embedded, no exposed local port, no browser taskbar grouping)
        if (start_wv(entry_point)) {
            if (on_ready) {
                try { on_ready(); } catch (...) {}
            }
            run();
        } else if (start(entry_point)) {
            // 2. Fallback: Installed Chromium browser app-mode (Edge, Chrome)
            if (on_ready) {
                try { on_ready(); } catch (...) {}
            }
            run();
        } else {
            std::cerr << "\n[AuraUI Error] Failed to launch window: Neither WebView2 nor a supported browser was found!\n"
                      << "[AuraUI Info] Please install the Microsoft Edge WebView2 runtime or a Chromium browser.\n" << std::endl;
#if defined(_WIN32)
            MessageBoxA(NULL,
                "AuraUI requires the Microsoft Edge WebView2 runtime or a modern web browser to display the interface.\n\n"
                "Neither WebView2 nor an installed browser (Microsoft Edge, Google Chrome, or Mozilla Firefox) was detected.\n\n"
                "Please install the Microsoft Edge WebView2 runtime.",
                "AuraUI - Runtime Required",
                MB_ICONERROR | MB_OK);
#endif
        }
    }

private:
    void register_pipeline(const std::string& path, const std::vector<Middleware>& middlewares, RouteHandler handler) {
        std::string clean = unicode::trim_slashes(path);
        register_route_name(clean);

        auto callback = [clean, middlewares, handler](webui::window::event* e) {
            Req req(clean, e);
            Res res;
            auto exec = std::make_shared<PipelineExecution>(middlewares, handler);
            exec->next(req, res);
            json out = res.to_json();
            try {
                e->return_string(out.dump(-1, ' ', false, json::error_handler_t::replace));
            } catch (...) {
                e->return_string("{\"error\":true}");
            }
        };

        // 1. Bind canonical name (e.g. "auth/login" or "calculate")
        m_win.bind(clean, callback);

        // 2. Bind with leading slash (e.g. "/auth/login")
        m_win.bind("/" + clean, callback);

        // 3. If nested path, also bind dot and underscore notations if no collision
        if (clean.find('/') != std::string::npos) {
            std::string dot_str = clean;
            std::replace(dot_str.begin(), dot_str.end(), '/', '.');
            if (m_registered_routes.find(dot_str) == m_registered_routes.end()) {
                m_registered_routes.insert(dot_str);
                m_win.bind(dot_str, callback);
            }

            std::string under_str = clean;
            std::replace(under_str.begin(), under_str.end(), '/', '_');
            if (m_registered_routes.find(under_str) == m_registered_routes.end()) {
                m_registered_routes.insert(under_str);
                m_win.bind(under_str, callback);
            }
        }
    }

    void mount_router_recursive(const std::string& prefix, const Router& router, std::vector<Middleware> accumulated_mws) {
        for (const auto& mw : router.middlewares()) {
            accumulated_mws.push_back(mw);
        }

        std::string clean_prefix = unicode::trim_slashes(prefix);

        for (const auto& entry : router.routes()) {
            std::string clean_route = unicode::trim_slashes(entry.path);
            std::string full_path = clean_prefix.empty() ? clean_route : (clean_prefix + "/" + clean_route);
            
            std::vector<Middleware> route_mws = accumulated_mws;
            for (const auto& rmw : entry.middlewares) {
                route_mws.push_back(rmw);
            }
            register_pipeline(full_path, route_mws, entry.handler);
        }

        for (const auto& sub : router.subrouters()) {
            std::string sub_prefix = unicode::trim_slashes(sub.first);
            std::string next_prefix = clean_prefix.empty() ? sub_prefix : (clean_prefix + "/" + sub_prefix);
            mount_router_recursive(next_prefix, sub.second, accumulated_mws);
        }
    }

    void bind_internal_handlers() {
        // Built-in window controls for custom title bars
        bind_action("__aura_win_minimize", [this]() {
            m_win.minimize();
        });

        bind_action("__aura_win_maximize", [this]() {
            m_win.maximize();
        });

        bind_action("__aura_win_close", [this]() {
            m_win.close();
        });

        // Built-in system info endpoint
        bind("__aura_sys_info", [](const Context&) -> json {
            return json{
                {"overview", system::get_system_overview()},
                {"memory", system::get_memory_info()}
            };
        });

        // Built-in Unicode wide-char file dialogs
        bind("__aura_dialog_open", [](const Context& ctx) -> json {
            std::string filter = ctx.get<std::string>("filter", "");
            std::string title = ctx.get<std::string>("title", "");
            std::string path = system::open_file_dialog(filter, title);
            return json{{"path", path}, {"cancelled", path.empty()}};
        });

        bind("__aura_dialog_save", [](const Context& ctx) -> json {
            std::string default_name = ctx.get<std::string>("default_name", "output.txt");
            std::string filter = ctx.get<std::string>("filter", "");
            std::string title = ctx.get<std::string>("title", "");
            std::string path = system::save_file_dialog(default_name, filter, title);
            return json{{"path", path}, {"cancelled", path.empty()}};
        });

        bind("__aura_dialog_folder", [](const Context& ctx) -> json {
            std::string title = ctx.get<std::string>("title", "Select Folder");
            std::string path = system::open_folder_dialog(title);
            return json{{"path", path}, {"cancelled", path.empty()}};
        });

        // Built-in Unicode clipboard APIs
        bind("__aura_clipboard_write", [](const Context& ctx) -> json {
            std::string text = ctx.get<std::string>("text", "");
            bool success = system::set_clipboard_text(text);
            return json{{"success", success}};
        });

        bind("__aura_clipboard_read", [this](const Context&) -> json {
            if (!m_clipboard_read_allowed) {
                return json{{"text", ""}, {"error", "Clipboard read access blocked by security policy"}};
            }
            std::string text = system::get_clipboard_text();
            return json{{"text", text}};
        });

        // Built-in shell execute (hardened allowlist)
        bind("__aura_shell_open", [](const Context& ctx) -> json {
            std::string target = ctx.get<std::string>("target", "");
            bool success = system::open_external(target);
            return json{{"success", success}};
        });

        // Built-in persistent state storage (saved under %APPDATA%/AuraUI/<AppName>/store.json)
        bind("__aura_store_get", [this](const Context& ctx) -> json {
            std::string key = ctx.get<std::string>("key", "");
            json data = load_storage();
            if (data.contains(key)) {
                return json{{"found", true}, {"value", data[key]}};
            }
            return json{{"found", false}, {"value", nullptr}};
        });

        bind("__aura_store_set", [this](const Context& ctx) -> json {
            std::string key = ctx.get<std::string>("key", "");
            if (key.empty()) return json{{"success", false}};
            json data = load_storage();
            if (ctx.body().contains("value")) {
                data[key] = ctx.body()["value"];
            } else {
                data[key] = nullptr;
            }
            bool ok = save_storage(data);
            return json{{"success", ok}};
        });

        bind("__aura_store_remove", [this](const Context& ctx) -> json {
            std::string key = ctx.get<std::string>("key", "");
            json data = load_storage();
            if (data.contains(key)) {
                data.erase(key);
                save_storage(data);
            }
            return json{{"success", true}};
        });

        bind("__aura_store_get_all", [this](const Context&) -> json {
            return load_storage();
        });
    }
};

} // namespace auraui
