#pragma once

#include "command.hpp"
#include "middleware.hpp"
#include "router.hpp"
#include "result.hpp"
#include "error.hpp"
#include "events.hpp"
#include "nlohmann/json.hpp"

#include <string>
#include <memory>
#include <cstdint>

namespace auraui {

struct AppOptions {
    std::string title{"AuraUI Application"};
    uint32_t width{1000};
    uint32_t height{700};
    uint32_t min_width{0};
    uint32_t min_height{0};
    bool frameless{false};
    bool resizable{true};
    std::string config_path{"aura.config.json"};
    std::string asset_path{""};
    bool dev_mode{false};
    bool debug{false};
};

class App {
private:
    class Impl;
    std::unique_ptr<Impl> m_impl;

public:
    explicit App(AppOptions options = AppOptions{});
    App(std::string title, uint32_t width = 1000, uint32_t height = 700);
    ~App();

    // Non-copyable, move-only
    App(const App&) = delete;
    App& operator=(const App&) = delete;
    App(App&&) noexcept;
    App& operator=(App&&) noexcept;

    // Register a command
    App& command(CommandSpec spec, CommandHandler handler);
    App& command(const std::string& name, CommandHandler handler);

    // Global middleware registration
    App& use(Middleware mw);

    // Mount modular subrouter
    App& mount(const std::string& prefix, const Router& router);

    // Thread-safe event emission to frontend
    void emit(const std::string& event, const nlohmann::json& payload);

    // Register embedded asset in memory
    void register_asset(std::string path, std::string data, std::string mime = "");
    void set_asset_path(std::filesystem::path path);

    // Window controls
    void set_title(const std::string& title);
    void set_size(uint32_t width, uint32_t height);
    void set_min_size(uint32_t width, uint32_t height);
    void set_center();
    void navigate(const std::string& path);
    void close();

    // Start UI and run event loop
    void start(std::string entry_point = "index.html");
    int run(const std::string& entry_point = "");

    template <typename... Args>
    void set_vfs(Args&&...) noexcept {}

    // Express-style routing aliases for convenience
    App& route(const std::string& name, CommandHandler handler) {
        return command(name, std::move(handler));
    }

    App& on(const std::string& name, CommandHandler handler) {
        return command(name, std::move(handler));
    }

    template <typename Func>
    App& bind(const std::string& name, Func&& func) {
        if constexpr (std::is_invocable_v<Func, Context, Reply>) {
            return command(name, std::forward<Func>(func));
        } else if constexpr (std::is_invocable_v<Func, const Context&>) {
            return command(name, [f = std::forward<Func>(func)](Context ctx, Reply reply) {
                try {
                    auto res = f(ctx);
                    reply.send(res);
                } catch (const std::exception& ex) {
                    reply.error(ErrorCode::internal, ex.what());
                } catch (...) {
                    reply.error(ErrorCode::internal, "Unknown exception in bound handler");
                }
            });
        } else if constexpr (std::is_invocable_v<Func, Context>) {
            return command(name, [f = std::forward<Func>(func)](Context ctx, Reply reply) {
                try {
                    auto res = f(ctx);
                    reply.send(res);
                } catch (const std::exception& ex) {
                    reply.error(ErrorCode::internal, ex.what());
                } catch (...) {
                    reply.error(ErrorCode::internal, "Unknown exception in bound handler");
                }
            });
        } else {
            return command(name, [f = std::forward<Func>(func)](Context, Reply reply) {
                try {
                    auto res = f();
                    reply.send(res);
                } catch (const std::exception& ex) {
                    reply.error(ErrorCode::internal, ex.what());
                } catch (...) {
                    reply.error(ErrorCode::internal, "Unknown exception in bound handler");
                }
            });
        }
    }

    int listen(const std::string& entry_point = "index.html") {
        return run(entry_point);
    }
};

} // namespace auraui
