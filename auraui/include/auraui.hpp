#pragma once

// ============================================================================
// AuraUI - Modern C++ Desktop Application Framework with HTML/CSS UI
// ============================================================================

#include "webui.hpp"
#include "nlohmann/json.hpp"

#include <string>
#include <string_view>
#include <functional>
#include <memory>
#include <unordered_map>
#include <vector>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <algorithm>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <commdlg.h>
#include <psapi.h>
#include <shellapi.h>
#include <shlobj.h>
#endif

#include <filesystem>
#include <thread>
#include <atomic>
#include <chrono>

namespace auraui {

using json = nlohmann::json;

// ----------------------------------------------------------------------------
// Request (Req) - Express-style incoming request object
// ----------------------------------------------------------------------------
class Request {
private:
    std::string m_path;
    json m_body;
    json m_locals;
    webui::window::event* m_event{nullptr};
    std::chrono::steady_clock::time_point m_start_time;

public:
    Request(webui::window::event* e = nullptr)
        : Request("", e) {}

    Request(const std::string& path, webui::window::event* e)
        : m_path(path), m_event(e), m_start_time(std::chrono::steady_clock::now())
    {
        m_locals = json::object();
        if (!e) {
            m_body = json::object();
            return;
        }

        size_t count = e->get_count();
        if (count == 0) {
            m_body = json::object();
        } else if (count == 1) {
            std::string arg = e->get_string(0);
            try {
                m_body = json::parse(arg);
            } catch (...) {
                m_body = arg;
            }
        } else {
            m_body = json::array();
            for (size_t i = 0; i < count; ++i) {
                std::string arg = e->get_string(i);
                try {
                    m_body.push_back(json::parse(arg));
                } catch (...) {
                    m_body.push_back(arg);
                }
            }
        }
    }

    const std::string& path() const { return m_path; }
    const json& data() const { return m_body; }
    const json& body() const { return m_body; }

    // Access payload directly via subscript: req["param"]
    json operator[](const std::string& key) const {
        if (m_body.is_object() && m_body.contains(key)) {
            return m_body[key];
        }
        return json(nullptr);
    }

    bool has(const std::string& key) const {
        return m_body.is_object() && m_body.contains(key);
    }

    template <typename T>
    T get(const std::string& key, const T& default_val = T{}) const {
        if (m_body.is_object() && m_body.contains(key)) {
            try {
                return m_body[key].get<T>();
            } catch (...) {
                return default_val;
            }
        }
        return default_val;
    }

    // Context storage for middleware state passing (e.g. auth user, session)
    void set(const std::string& key, const json& val) {
        m_locals[key] = val;
    }

    json get_local(const std::string& key) const {
        if (m_locals.contains(key)) return m_locals[key];
        return json(nullptr);
    }

    template <typename T>
    T get_local(const std::string& key, const T& default_val = T{}) const {
        if (m_locals.contains(key)) {
            try {
                return m_locals[key].get<T>();
            } catch (...) {}
        }
        return default_val;
    }

    // High-resolution timing metrics
    int64_t elapsed_us() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now - m_start_time).count();
    }

    double elapsed_ms() const {
        return (double)elapsed_us() / 1000.0;
    }

    webui::window::event* raw_event() const { return m_event; }
};

// Aliases for modern Express syntax and backward compatibility
using Req = Request;
using Context = Request;

// ----------------------------------------------------------------------------
// Response (Res) - Express-style fluent response builder
// ----------------------------------------------------------------------------
class Response {
private:
    int m_status{200};
    auraui::json m_body{nullptr};
    bool m_sent{false};

public:
    Response() = default;

    Response& status(int code) {
        m_status = code;
        return *this;
    }

    int get_status() const { return m_status; }
    bool is_sent() const { return m_sent; }

    Response& json(const auraui::json& payload) {
        m_body = payload;
        m_sent = true;
        return *this;
    }

    Response& send(const auraui::json& payload) {
        return json(payload);
    }

    Response& text(const std::string& message) {
        m_body = message;
        m_sent = true;
        return *this;
    }

    Response& ok(const auraui::json& data = auraui::json::object()) {
        m_status = 200;
        if (data.is_object()) {
            auraui::json res_obj = data;
            if (!res_obj.contains("status")) res_obj["status"] = "success";
            if (!res_obj.contains("success")) res_obj["success"] = true;
            m_body = res_obj;
        } else {
            m_body = auraui::json{
                {"success", true},
                {"status", 200},
                {"data", data}
            };
        }
        m_sent = true;
        return *this;
    }

    Response& error(const std::string& message, int code = 400, const auraui::json& details = auraui::json::object()) {
        m_status = code;
        m_body = auraui::json{
            {"success", false},
            {"status", code},
            {"error", message}
        };
        if (!details.empty()) {
            m_body["details"] = details;
        }
        m_sent = true;
        return *this;
    }

    auraui::json to_json() const {
        if (!m_sent) {
            return auraui::json{{"success", true}, {"status", m_status}};
        }
        return m_body;
    }
};

using Res = Response;

// ----------------------------------------------------------------------------
// Middleware Pipeline & Execution Model
// ----------------------------------------------------------------------------
using Next = std::function<void()>;
using Middleware = std::function<void(Req&, Res&, Next)>;
using RouteHandler = std::function<void(Req&, Res&)>;
using SimpleHandler = std::function<json(Req&)>;

struct PipelineExecution : std::enable_shared_from_this<PipelineExecution> {
    std::vector<Middleware> middlewares;
    RouteHandler final_handler;
    size_t index{0};

    PipelineExecution(std::vector<Middleware> mws, RouteHandler handler)
        : middlewares(std::move(mws)), final_handler(std::move(handler)) {}

    void next(Req& req, Res& res) {
        if (res.is_sent()) return;
        if (index < middlewares.size()) {
            size_t cur = index++;
            auto self = shared_from_this();
            try {
                middlewares[cur](req, res, [self, &req, &res]() {
                    self->next(req, res);
                });
            } catch (const std::exception& ex) {
                res.error(std::string("Middleware error: ") + ex.what(), 500);
            } catch (...) {
                res.error("Unknown middleware error", 500);
            }
        } else {
            if (!res.is_sent()) {
                try {
                    final_handler(req, res);
                    if (!res.is_sent()) {
                        res.ok();
                    }
                } catch (const std::exception& ex) {
                    res.error(std::string("Handler error: ") + ex.what(), 500);
                } catch (...) {
                    res.error("Unknown handler error", 500);
                }
            }
        }
    }
};

// ----------------------------------------------------------------------------
// Router - Modular route grouping and sub-routing
// ----------------------------------------------------------------------------
class Router {
public:
    struct RouteEntry {
        std::string path;
        RouteHandler handler;
        std::vector<Middleware> middlewares;
    };

private:
    std::vector<Middleware> m_middlewares;
    std::vector<RouteEntry> m_routes;
    std::vector<std::pair<std::string, Router>> m_subrouters;

public:
    Router() = default;

    Router& use(Middleware mw) {
        m_middlewares.push_back(std::move(mw));
        return *this;
    }

    Router& use(const std::string& prefix, const Router& subrouter) {
        m_subrouters.emplace_back(prefix, subrouter);
        return *this;
    }

    Router& route(const std::string& path, RouteHandler handler) {
        m_routes.push_back({path, std::move(handler), {}});
        return *this;
    }

    Router& route(const std::string& path, SimpleHandler handler) {
        return route(path, [handler](Req& req, Res& res) {
            json out = handler(req);
            res.ok(out);
        });
    }

    Router& route(const std::string& path, Middleware mw, RouteHandler handler) {
        m_routes.push_back({path, std::move(handler), {std::move(mw)}});
        return *this;
    }

    Router& on(const std::string& path, RouteHandler handler) {
        return route(path, std::move(handler));
    }

    Router& on(const std::string& path, SimpleHandler handler) {
        return route(path, std::move(handler));
    }

    const std::vector<Middleware>& middlewares() const { return m_middlewares; }
    const std::vector<RouteEntry>& routes() const { return m_routes; }
    const std::vector<std::pair<std::string, Router>>& subrouters() const { return m_subrouters; }
};

// ----------------------------------------------------------------------------
// Built-in Middleware Suite
// ----------------------------------------------------------------------------
namespace middleware {

inline Middleware logger() {
    return [](Req& req, Res& res, Next next) {
        auto t0 = std::chrono::steady_clock::now();
        next();
        auto t1 = std::chrono::steady_clock::now();
        auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

        int status = res.get_status();
        const char* color_code = (status >= 400) ? "\033[31m" : (status >= 300) ? "\033[33m" : "\033[32m";
        const char* reset_code = "\033[0m";

        std::cout << "[AuraUI] " << color_code << status << reset_code 
                  << " " << req.path() 
                  << " (" << elapsed_us << " \xC2\xB5s)" << std::endl;
    };
}

inline Middleware timing() {
    return [](Req& req, Res& res, Next next) {
        next();
        json j = res.to_json();
        if (j.is_object() && !j.contains("_time_us")) {
            j["_time_us"] = req.elapsed_us();
            res.json(j);
        }
    };
}

inline Middleware guard(std::function<bool(const Req&)> validator, const std::string& err_msg = "Access Forbidden") {
    return [validator, err_msg](Req& req, Res& res, Next next) {
        if (validator(req)) {
            next();
        } else {
            res.error(err_msg, 403);
        }
    };
}

} // namespace middleware

// ----------------------------------------------------------------------------
// System Utility Helpers for Win32 (CPU, RAM, Disks, Dialogs)
// ----------------------------------------------------------------------------
namespace system {

inline json get_memory_info() {
#if defined(_WIN32)
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    if (GlobalMemoryStatusEx(&memInfo)) {
        double total_gb = (double)memInfo.ullTotalPhys / (1024.0 * 1024.0 * 1024.0);
        double avail_gb = (double)memInfo.ullAvailPhys / (1024.0 * 1024.0 * 1024.0);
        double used_gb  = total_gb - avail_gb;
        int load_pct    = (int)memInfo.dwMemoryLoad;

        return json{
            {"total_gb", total_gb},
            {"used_gb", used_gb},
            {"avail_gb", avail_gb},
            {"percent_used", load_pct}
        };
    }
#endif
    return json{{"error", "Not supported on this platform"}};
}

inline json get_system_overview() {
#if defined(_WIN32)
    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    char computerName[MAX_COMPUTERNAME_LENGTH + 1];
    DWORD size = sizeof(computerName);
    GetComputerNameA(computerName, &size);

    return json{
        {"os", "Windows"},
        {"computer_name", std::string(computerName)},
        {"cpu_cores", (int)sysInfo.dwNumberOfProcessors},
        {"arch", (sysInfo.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64) ? "x64" : "x86"}
    };
#else
    return json{{"os", "Other"}};
#endif
}

inline std::string open_file_dialog(const std::string& filter = "All Files (*.*)\0*.*\0") {
#if defined(_WIN32)
    char filename[MAX_PATH] = "";
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;

    // Security fix: Ensure buffer is strictly double-null terminated to prevent out-of-bounds heap/stack read
    std::vector<char> filter_buf;
    if (filter.empty()) {
        const char def_filter[] = "All Files (*.*)\0*.*\0\0";
        filter_buf.assign(def_filter, def_filter + sizeof(def_filter));
    } else {
        filter_buf.assign(filter.begin(), filter.end());
        filter_buf.push_back('\0');
        filter_buf.push_back('\0');
    }
    ofn.lpstrFilter = filter_buf.data();
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
    ofn.lpstrDefExt = "";

    if (GetOpenFileNameA(&ofn)) {
        return std::string(filename);
    }
#endif
    return "";
}

inline std::string save_file_dialog(const std::string& default_name = "output.txt") {
#if defined(_WIN32)
    char filename[MAX_PATH] = "";
    if (!default_name.empty() && default_name.size() < MAX_PATH) {
        strncpy_s(filename, default_name.c_str(), sizeof(filename) - 1);
    }
    OPENFILENAMEA ofn;
    ZeroMemory(&ofn, sizeof(ofn));
    ofn.lStructSize = sizeof(ofn);
    ofn.hwndOwner = NULL;

    const char filter_str[] = "All Files (*.*)\0*.*\0Text Files (*.txt)\0*.txt\0\0";
    ofn.lpstrFilter = filter_str;
    ofn.lpstrFile = filename;
    ofn.nMaxFile = MAX_PATH;
    ofn.Flags = OFN_EXPLORER | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT;

    if (GetSaveFileNameA(&ofn)) {
        return std::string(filename);
    }
#endif
    return "";
}

inline std::string open_folder_dialog(const std::string& title = "Select Folder") {
#if defined(_WIN32)
    char path[MAX_PATH] = "";
    BROWSEINFOA bi = { 0 };
    bi.lpszTitle = title.c_str();
    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_NEWDIALOGSTYLE;
    LPITEMIDLIST pidl = SHBrowseForFolderA(&bi);
    if (pidl != 0) {
        if (SHGetPathFromIDListA(pidl, path)) {
            CoTaskMemFree(pidl);
            return std::string(path);
        }
        CoTaskMemFree(pidl);
    }
#endif
    return "";
}

inline bool set_clipboard_text(const std::string& text) {
#if defined(_WIN32)
    if (!OpenClipboard(NULL)) return false;
    EmptyClipboard();

    // Security & correctness: Convert UTF-8 to UTF-16 WideChar for full Unicode & Emoji support
    int wlen = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, NULL, 0);
    if (wlen <= 0) {
        CloseClipboard();
        return false;
    }

    HGLOBAL hGlob = GlobalAlloc(GMEM_MOVEABLE, wlen * sizeof(wchar_t));
    if (!hGlob) { CloseClipboard(); return false; }

    wchar_t* pMem = (wchar_t*)GlobalLock(hGlob);
    if (!pMem) { GlobalFree(hGlob); CloseClipboard(); return false; }

    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, pMem, wlen);
    GlobalUnlock(hGlob);

    SetClipboardData(CF_UNICODETEXT, hGlob);
    CloseClipboard();
    return true;
#else
    return false;
#endif
}

inline std::string get_clipboard_text() {
#if defined(_WIN32)
    if (!OpenClipboard(NULL)) return "";
    HANDLE hData = GetClipboardData(CF_UNICODETEXT);
    if (!hData) {
        // Fallback to ANSI text if Unicode clipboard format is not available
        hData = GetClipboardData(CF_TEXT);
        if (!hData) { CloseClipboard(); return ""; }
        char* pszText = (char*)GlobalLock(hData);
        if (!pszText) { CloseClipboard(); return ""; }
        std::string result(pszText);
        GlobalUnlock(hData);
        CloseClipboard();
        return result;
    }

    wchar_t* pszWText = (wchar_t*)GlobalLock(hData);
    if (!pszWText) { CloseClipboard(); return ""; }

    // Convert UTF-16 WideChar to UTF-8
    int utf8_len = WideCharToMultiByte(CP_UTF8, 0, pszWText, -1, NULL, 0, NULL, NULL);
    std::string result;
    if (utf8_len > 0) {
        result.resize(utf8_len - 1);
        WideCharToMultiByte(CP_UTF8, 0, pszWText, -1, &result[0], utf8_len, NULL, NULL);
    }

    GlobalUnlock(hData);
    CloseClipboard();
    return result;
#else
    return "";
#endif
}

// Security: Prevent Arbitrary Command Execution via ShellExecuteA
inline bool open_external(const std::string& target) {
#if defined(_WIN32)
    if (target.empty() || target.size() > 2048) return false;

    // Reject control characters or newlines
    for (char c : target) {
        if (static_cast<unsigned char>(c) < 32) return false;
    }

    // 1. Whitelist safe web & mail protocols
    if (target.rfind("https://", 0) == 0 || 
        target.rfind("http://", 0) == 0 || 
        target.rfind("mailto:", 0) == 0) 
    {
        HINSTANCE res = ShellExecuteA(NULL, "open", target.c_str(), NULL, NULL, SW_SHOWNORMAL);
        return ((INT_PTR)res > 32);
    }

    // 2. Reject hazardous UNC network paths (e.g. \\attacker.com\share\payload.exe)
    if (target.rfind("\\\\", 0) == 0 || target.rfind("//", 0) == 0) {
        return false;
    }

    // 3. Block executable and script file extensions
    std::string lower = target;
    std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c){ return static_cast<char>(::tolower(c)); });

    const char* blocked_exts[] = {
        ".exe", ".bat", ".cmd", ".ps1", ".vbs", ".vbe", ".js", ".jse", 
        ".wsf", ".wsh", ".msc", ".msi", ".msp", ".com", ".scr", ".pif", 
        ".reg", ".hta", ".cpl", ".jar", ".lnk", ".inf", ".url"
    };
    for (const char* ext : blocked_exts) {
        size_t ext_len = strlen(ext);
        if (lower.size() >= ext_len && 
            lower.compare(lower.size() - ext_len, ext_len, ext) == 0) {
            return false;
        }
    }

    // 4. If target is an existing local directory or safe document, open it
    namespace fs = std::filesystem;
    std::error_code ec;
    if (fs::exists(target, ec)) {
        HINSTANCE res = ShellExecuteA(NULL, "open", target.c_str(), NULL, NULL, SW_SHOWNORMAL);
        return ((INT_PTR)res > 32);
    }

    return false;
#else
    return false;
#endif
}

} // namespace system

// ----------------------------------------------------------------------------
// Main Application Class
// ----------------------------------------------------------------------------
class App {
public:
    using JsonHandler = std::function<json(const Context&)>;
    using VoidHandler = std::function<void()>;

private:
    webui::window m_win;
    std::string m_title;
    bool m_is_frameless{false};
    std::atomic<bool> m_hot_reload_running{false};
    std::thread m_hot_reload_thread;
    std::vector<Middleware> m_global_middlewares;
    bool m_clipboard_read_allowed{true};

public:
    App(const std::string& title = "AuraUI Application", unsigned int width = 1000, unsigned int height = 700)
        : m_title(title)
    {
        m_win.set_size(width, height);
        m_win.set_center();

        // Built-in window controls
        bind_internal_handlers();
    }

    ~App() {
        stop_hot_reload();
    }

    // Live Hot-Reload Engine for Development
    void enable_hot_reload(const std::string& watch_folder = "ui") {
        namespace fs = std::filesystem;
        if (!fs::exists(watch_folder) || !fs::is_directory(watch_folder)) return;
        if (m_hot_reload_running) return;

        m_hot_reload_running = true;
        m_hot_reload_thread = std::thread([this, watch_folder]() {
            namespace fs = std::filesystem;
            auto get_max_mtime = [](const std::string& dir) {
                fs::file_time_type max_time = (fs::file_time_type::min)();
                try {
                    for (const auto& entry : fs::recursive_directory_iterator(dir)) {
                        if (entry.is_regular_file()) {
                            auto t = entry.last_write_time();
                            if (t > max_time) max_time = t;
                        }
                    }
                } catch (...) {}
                return max_time;
            };

            auto last_time = get_max_mtime(watch_folder);
            std::cout << "[AuraUI Dev] Hot-reload active on '" << watch_folder << "/'\n";

            while (m_hot_reload_running) {
                std::this_thread::sleep_for(std::chrono::milliseconds(300));
                if (!m_hot_reload_running) break;

                auto current_time = get_max_mtime(watch_folder);
                if (current_time > last_time) {
                    last_time = current_time;
                    std::cout << "[AuraUI Dev] File change detected! Refreshing window...\n";
                    m_win.run("location.reload();");
                }
            }
        });
    }

    void stop_hot_reload() {
        if (m_hot_reload_running) {
            m_hot_reload_running = false;
            if (m_hot_reload_thread.joinable()) {
                m_hot_reload_thread.detach(); // detach so window closing doesn't block
            }
        }
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

    // Expose a C++ JSON lambda callable from JS:
    // JS: const res = await aura.call("calc", { x: 5, y: 10 });
    App& bind(const std::string& name, JsonHandler handler) {
        m_win.bind(name, [handler](webui::window::event* e) {
            Context ctx(e);
            json result;
            try {
                result = handler(ctx);
            } catch (const std::exception& ex) {
                result = json{{"error", true}, {"message", ex.what()}};
            } catch (...) {
                result = json{{"error", true}, {"message", "Unknown C++ exception"}};
            }
            e->return_string(result.dump());
        });
        return *this;
    }

    // Expose a simple void function
    App& bind_action(const std::string& name, VoidHandler handler) {
        m_win.bind(name, [handler](webui::window::event* e) {
            try {
                handler();
            } catch (...) {}
            e->return_bool(true);
        });
        return *this;
    }

    // Send an event to the frontend:
    // C++: app.emit("statusUpdate", { {"progress", 50} });
    // JS:  aura.on("statusUpdate", (data) => console.log(data));
    void emit(const std::string& event_name, const json& payload) {
        // Security fix: Use json().dump() to escape event_name and prevent JavaScript injection
        std::string script = "if (window.aura && window.aura.__dispatch) { window.aura.__dispatch(" + 
                             json(event_name).dump() + ", " + payload.dump() + "); }";
        m_win.run(script);
    }

    // Execute arbitrary JavaScript code
    void eval_js(const std::string& script) {
        m_win.run(script);
    }

    // Programmatically navigate frontend view routes from C++
    void navigate(const std::string& route) {
        // Security fix: Use json().dump() to escape route and prevent JavaScript injection
        eval_js("if (window.aura && window.aura.router) { window.aura.router.navigate(" + 
                json(route).dump() + "); }");
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

    // Fluent Express-style listen (starts UI, hot-reload in dev, and enters loop)
    void listen(const std::string& entry_point = "index.html", std::function<void()> on_ready = nullptr) {
        print_banner();
        if (start(entry_point)) {
            if (on_ready) {
                try { on_ready(); } catch (...) {}
            }
            run();
        } else if (start_wv(entry_point)) {
            // Fallback: Try in-process WebView2 runtime if standalone browser is unavailable
            if (on_ready) {
                try { on_ready(); } catch (...) {}
            }
            run();
        } else {
            std::cerr << "\n[AuraUI Error] Failed to launch window: No supported browser detected!\n"
                      << "[AuraUI Info] Please install Microsoft Edge, Google Chrome, or Mozilla Firefox.\n" << std::endl;
#if defined(_WIN32)
            MessageBoxA(NULL,
                "AuraUI requires a modern web browser to display the application interface.\n\n"
                "No supported browser was detected on this system (Microsoft Edge, Google Chrome, or Mozilla Firefox).\n\n"
                "Please install Microsoft Edge or Google Chrome to run this application.",
                "AuraUI - Browser Required",
                MB_ICONERROR | MB_OK);
#endif
        }
    }

private:
    static std::string trim_slashes(const std::string& str) {
        size_t start = 0;
        while (start < str.size() && str[start] == '/') ++start;
        size_t end = str.size();
        while (end > start && str[end - 1] == '/') --end;
        return str.substr(start, end - start);
    }

    void register_pipeline(const std::string& path, const std::vector<Middleware>& middlewares, RouteHandler handler) {
        std::string clean = trim_slashes(path);
        auto callback = [clean, middlewares, handler](webui::window::event* e) {
            Req req(clean, e);
            Res res;
            auto exec = std::make_shared<PipelineExecution>(middlewares, handler);
            exec->next(req, res);
            json out = res.to_json();
            e->return_string(out.dump());
        };

        // 1. Bind canonical name (e.g. "auth/login" or "calculate")
        m_win.bind(clean, callback);

        // 2. Bind with leading slash (e.g. "/auth/login")
        m_win.bind("/" + clean, callback);

        // 3. If nested path, also bind dot and underscore notations
        if (clean.find('/') != std::string::npos) {
            std::string dot_str = clean;
            std::replace(dot_str.begin(), dot_str.end(), '/', '.');
            m_win.bind(dot_str, callback);

            std::string under_str = clean;
            std::replace(under_str.begin(), under_str.end(), '/', '_');
            m_win.bind(under_str, callback);
        }
    }

    void mount_router_recursive(const std::string& prefix, const Router& router, std::vector<Middleware> accumulated_mws) {
        for (const auto& mw : router.middlewares()) {
            accumulated_mws.push_back(mw);
        }

        std::string clean_prefix = trim_slashes(prefix);

        for (const auto& entry : router.routes()) {
            std::string clean_route = trim_slashes(entry.path);
            std::string full_path = clean_prefix.empty() ? clean_route : (clean_prefix + "/" + clean_route);
            
            std::vector<Middleware> route_mws = accumulated_mws;
            for (const auto& rmw : entry.middlewares) {
                route_mws.push_back(rmw);
            }
            register_pipeline(full_path, route_mws, entry.handler);
        }

        for (const auto& sub : router.subrouters()) {
            std::string sub_prefix = trim_slashes(sub.first);
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

        // Built-in file dialogs
        bind("__aura_dialog_open", [](const Context& ctx) -> json {
            std::string filter = ctx.get<std::string>("filter", "All Files (*.*)\0*.*\0");
            std::string path = system::open_file_dialog(filter);
            return json{{"path", path}, {"cancelled", path.empty()}};
        });

        bind("__aura_dialog_save", [](const Context& ctx) -> json {
            std::string default_name = ctx.get<std::string>("default_name", "output.txt");
            std::string path = system::save_file_dialog(default_name);
            return json{{"path", path}, {"cancelled", path.empty()}};
        });

        bind("__aura_dialog_folder", [](const Context& ctx) -> json {
            std::string title = ctx.get<std::string>("title", "Select Folder");
            std::string path = system::open_folder_dialog(title);
            return json{{"path", path}, {"cancelled", path.empty()}};
        });

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

        bind("__aura_shell_open", [](const Context& ctx) -> json {
            std::string target = ctx.get<std::string>("target", "");
            bool success = system::open_external(target);
            return json{{"success", success}};
        });
    }
};

} // namespace auraui
