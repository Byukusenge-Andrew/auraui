#include <doctest/doctest.h>
#include "core/dispatcher.hpp"
#include <future>

using namespace auraui;
using namespace auraui::core;
using namespace auraui::security;

TEST_CASE("Dispatcher - Successful invocation") {
    CapabilityPolicy policy(nlohmann::json{{"system.info", true}});
    auto pool = std::make_shared<ThreadPool>(2);
    Dispatcher dispatcher(policy, 100, pool);

    CommandSpec spec("calc.add");
    dispatcher.register_command(spec, [](Context ctx, Reply reply) {
        int a = ctx.get<int>("a", 0);
        int b = ctx.get<int>("b", 0);
        reply.send({{"sum", a + b}});
    });

    std::promise<std::string> prom;
    auto fut = prom.get_future();

    std::string req = R"({"v":1,"id":"req_1","cmd":"calc.add","args":{"a":7,"b":5}})";
    dispatcher.dispatch(req, [&prom](std::string res) {
        prom.set_value(res);
    });

    REQUIRE(fut.wait_for(std::chrono::seconds(2)) == std::future_status::ready);
    std::string response = fut.get();
    nlohmann::json parsed = nlohmann::json::parse(response);
    CHECK(parsed["ok"] == true);
    CHECK(parsed["data"]["sum"] == 12);
}

TEST_CASE("Dispatcher - Capability denial") {
    // Empty policy -> every capability denied
    CapabilityPolicy policy(nlohmann::json::object());
    auto pool = std::make_shared<ThreadPool>(2);
    Dispatcher dispatcher(policy, 100, pool);

    CommandSpec spec("clipboard.read");
    spec.with_capability("clipboard.read");
    dispatcher.register_command(spec, [](Context, Reply reply) {
        reply.send({{"text", "secret"}});
    });

    std::promise<std::string> prom;
    auto fut = prom.get_future();

    std::string req = R"({"v":1,"id":"req_2","cmd":"clipboard.read","args":{}})";
    dispatcher.dispatch(req, [&prom](std::string res) {
        prom.set_value(res);
    });

    REQUIRE(fut.wait_for(std::chrono::seconds(2)) == std::future_status::ready);
    std::string response = fut.get();
    nlohmann::json parsed = nlohmann::json::parse(response);
    CHECK(parsed["ok"] == false);
    CHECK(parsed["error"]["code"] == "permission_denied");
}

TEST_CASE("Dispatcher - Collision rejection") {
    CapabilityPolicy policy(nlohmann::json::object());
    Dispatcher dispatcher(policy);

    CHECK(dispatcher.register_command(CommandSpec("test.ping"), [](Context, Reply r){ r.send({}); }).has_value());
    // Second registration with same name must fail
    CHECK_FALSE(dispatcher.register_command(CommandSpec("test.ping"), [](Context, Reply r){ r.send({}); }).has_value());
}
