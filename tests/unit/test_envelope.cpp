#include <doctest/doctest.h>
#include "core/ipc_envelope.hpp"

using namespace auraui;
using namespace auraui::core;

TEST_CASE("IpcEnvelope - Valid parsing") {
    std::string json_req = R"({"v":1,"id":"req_001","cmd":"system.info","args":{"verbose":true}})";
    auto res = IpcEnvelope::parse_request(json_req);
    REQUIRE(res.has_value());
    CHECK(res->version == 1);
    CHECK(res->id == "req_001");
    CHECK(res->cmd == "system.info");
    CHECK(res->args["verbose"] == true);
}

TEST_CASE("IpcEnvelope - Version validation") {
    std::string json_v2 = R"({"v":2,"id":"req_001","cmd":"ping","args":{}})";
    auto res = IpcEnvelope::parse_request(json_v2);
    CHECK_FALSE(res.has_value());
    CHECK(res.error().code() == ErrorCode::unsupported);

    std::string json_no_v = R"({"id":"req_001","cmd":"ping","args":{}})";
    auto res2 = IpcEnvelope::parse_request(json_no_v);
    CHECK_FALSE(res2.has_value());
    CHECK(res2.error().code() == ErrorCode::invalid_argument);
}

TEST_CASE("IpcEnvelope - ID length boundary") {
    std::string long_id(65, 'a');
    std::string json_long = "{\"v\":1,\"id\":\"" + long_id + "\",\"cmd\":\"ping\",\"args\":{}}";
    auto res = IpcEnvelope::parse_request(json_long);
    CHECK_FALSE(res.has_value());
    CHECK(res.error().code() == ErrorCode::invalid_argument);

    std::string valid_id(64, 'a');
    std::string json_valid = "{\"v\":1,\"id\":\"" + valid_id + "\",\"cmd\":\"ping\",\"args\":{}}";
    auto res2 = IpcEnvelope::parse_request(json_valid);
    CHECK(res2.has_value());
}

TEST_CASE("IpcEnvelope - Safe response serialization") {
    IpcResponse resp = IpcResponse::success("req_100", {{"key", "value"}});
    std::string serialized = IpcEnvelope::serialize_response(resp);
    CHECK(serialized.find("\"ok\":true") != std::string::npos);
    CHECK(serialized.find("\"key\":\"value\"") != std::string::npos);

    IpcResponse err_resp = IpcResponse::fail("req_101", Error(ErrorCode::permission_denied, "Forbidden"));
    std::string err_serialized = IpcEnvelope::serialize_response(err_resp);
    CHECK(err_serialized.find("\"ok\":false") != std::string::npos);
    CHECK(err_serialized.find("\"permission_denied\"") != std::string::npos);
}
