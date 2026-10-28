#include "auraui.hpp"

int main() {
    // 1. Initialize modern AuraUI application window
    auraui::AppOptions opts;
    opts.title = "AuraUI Starter Application";
    opts.width = 1050;
    opts.height = 720;
    opts.min_width = 800;
    opts.min_height = 500;
    opts.config_path = "aura.config.json";
    opts.asset_path = "ui";

    auraui::App app(opts);

    // 2. Global Middleware: High-precision route logger
    app.use(auraui::middleware::logger());

    // 3. Interactive Counter RPC (In-Process C++ state)
    // Invoked by frontend: await aura.call('counter.increment')
    int counter = 0;
    app.bind("counter.increment", [&counter](const auraui::Context&) {
        counter++;
        return auraui::json{
            {"count", counter}
        };
    });

    // 4. Compute Pipeline RPC (Type-safe parameter extraction)
    // Invoked by frontend: await aura.call('calculate', { a, b, operation })
    app.bind("calculate", [](const auraui::Context& ctx) {
        double a = ctx.get<double>("a", 0.0);
        double b = ctx.get<double>("b", 0.0);
        std::string op = ctx.get<std::string>("operation", "+");

        double result = 0.0;
        if (op == "+") result = a + b;
        else if (op == "-") result = a - b;
        else if (op == "*") result = a * b;
        else if (op == "/" && b != 0.0) {
            result = a / b;
        }

        return auraui::json{
            {"operation", op},
            {"result", result}
        };
    });

    // 5. System Status RPC
    app.bind("api.ping", [](const auraui::Context&) {
        return auraui::json{
            {"message", "pong"},
            {"status", "online"}
        };
    });

    // 6. Launch native Webview window and enter event loop
    return app.run("index.html");
}
