#include "auraui.hpp"

int main() {
    // 1. Initialize modern AuraUI 2 application window
    auraui::AppOptions opts;
    opts.title = "My AuraUI Application";
    opts.width = 1000;
    opts.height = 680;
    opts.min_width = 800;
    opts.min_height = 500;
    opts.config_path = "aura.config.json";
    opts.asset_path = "ui";

    auraui::App app(opts);

    // 2. Global Middleware: Microsecond route logger
    app.use(auraui::middleware::logger());

    // 3. Command Binding returning JSON response
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

    // 4. API Ping Endpoint
    app.bind("api.ping", [](const auraui::Context&) {
        return auraui::json{
            {"message", "pong"},
            {"status", "online"}
        };
    });

    // 5. Start UI & run event loop
    return app.run("index.html");
}
