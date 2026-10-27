#include <doctest/doctest.h>
#include "core/dispatcher.hpp"
#include <future>

using namespace auraui;
using namespace auraui::core;
using namespace auraui::security;

TEST_CASE("Middleware - Async execution pipeline") {
    CapabilityPolicy policy(nlohmann::json::object());
    auto pool = std::make_shared<ThreadPool>(2);
    Dispatcher dispatcher(policy, 100, pool);

    // Global middleware that sets state
    dispatcher.use([](Context& ctx, Next next) {
        ctx.set_local("authenticated", true);
        next();
    });

    // Route-specific guard middleware
    auto auth_guard = [](Context& ctx, Next next) {
        if (ctx.get_local("authenticated") == true) {
            next();
        } else {
            next(Error(ErrorCode::permission_denied, "Unauthenticated"));
        }
    };

    CommandSpec spec("secure.data");
    dispatcher.register_command(spec, [](Context, Reply reply) {
        reply.send({{"secret", 42}});
    }, {auth_guard});

    std::promise<std::string> prom;
    auto fut = prom.get_future();

    dispatcher.dispatch(R"({"v":1,"id":"req_auth","cmd":"secure.data","args":{}})", [&prom](std::string res) {
        prom.set_value(res);
    });

    REQUIRE(fut.wait_for(std::chrono::seconds(2)) == std::future_status::ready);
    std::string response = fut.get();
    nlohmann::json parsed = nlohmann::json::parse(response);
    CHECK(parsed["ok"] == true);
    CHECK(parsed["data"]["secret"] == 42);
}
