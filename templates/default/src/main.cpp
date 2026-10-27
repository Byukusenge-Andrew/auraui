#include "auraui.hpp"

#if __has_include("vfs.h")
#include "vfs.h"
#define HAS_EMBEDDED_VFS 1
#endif

int main() {
    // 1. Initialize modern desktop application window (1000x680)
    auraui::App app("My AuraUI Application", 1000, 680);
    app.set_min_size(800, 500);

    // 2. Global Middleware: Microsecond route logger with ANSI color
    app.use(auraui::middleware::logger());

    // 3. Express-style Route Handler with Req and Res
    app.route("calculate", [](auraui::Req& req, auraui::Res& res) {
        double a = req.get<double>("a", 0.0);
        double b = req.get<double>("b", 0.0);
        std::string op = req.get<std::string>("operation", "+");

        double result = 0.0;
        if (op == "+") result = a + b;
        else if (op == "-") result = a - b;
        else if (op == "*") result = a * b;
        else if (op == "/" && b != 0.0) result = a / b;
        else if (op == "/" && b == 0.0) {
            return res.error("Division by zero", 400);
        }

        res.ok({
            {"operation", op},
            {"result", result}
        });
    });

    // 4. Express-style Sub-Router mounted at /api
    auraui::Router api;
    api.route("ping", [](auraui::Req& req, auraui::Res& res) {
        res.ok({
            {"message", "pong"},
            {"elapsed_us", req.elapsed_us()}
        });
    });
    app.use("api", api);

    // 5. Embedded Virtual File System (Production Single-Exe)
#if !defined(AURA_DEV_MODE) && defined(HAS_EMBEDDED_VFS)
    app.set_vfs(aura_vfs_handler);
#endif

    // 6. Express-style Fluent Listen: Starts UI, hot-reload, and event loop
    app.listen("index.html");
    return 0;
}
