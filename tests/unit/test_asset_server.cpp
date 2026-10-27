#include <doctest/doctest.h>
#include "assets/asset_store.hpp"
#include "assets/asset_server.hpp"
#include "httplib/httplib.h"

using namespace auraui::assets;

TEST_CASE("AssetServer - Loopback serving and security headers") {
    auto store = std::make_shared<AssetStore>();
    store->register_embedded("index.html", "<h1>Hello AuraUI</h1>", "text/html; charset=utf-8");
    store->register_embedded("styles.css", "body { margin: 0; }", "text/css; charset=utf-8");

    AssetServer server;
    std::string csp = "default-src 'self'; script-src 'self'";
    auto res = server.start(store, csp);
    REQUIRE(res.has_value());
    REQUIRE(server.is_running());
    REQUIRE(server.port() > 0);

    httplib::Client client("127.0.0.1", server.port());

    // 1. Fetch index.html
    auto resp = client.Get("/index.html");
    REQUIRE(resp != nullptr);
    CHECK(resp->status == 200);
    CHECK(resp->body == "<h1>Hello AuraUI</h1>");
    CHECK(resp->get_header_value("Content-Type") == "text/html; charset=utf-8");
    CHECK(resp->get_header_value("Content-Security-Policy") == csp);
    CHECK(resp->get_header_value("X-Content-Type-Options") == "nosniff");
    CHECK(resp->get_header_value("X-Frame-Options") == "DENY");

    // 2. Fetch root / (must serve index.html)
    auto root_resp = client.Get("/");
    REQUIRE(root_resp != nullptr);
    CHECK(root_resp->status == 200);
    CHECK(root_resp->body == "<h1>Hello AuraUI</h1>");

    // 3. ETag and 304 Not Modified
    std::string etag = resp->get_header_value("ETag");
    CHECK_FALSE(etag.empty());

    httplib::Headers headers = {{"If-None-Match", etag}};
    auto cached_resp = client.Get("/index.html", headers);
    REQUIRE(cached_resp != nullptr);
    CHECK(cached_resp->status == 304);

    // 4. DNS Rebinding Protection: Host header mismatch
    httplib::Headers spoofed_headers = {{"Host", "evil-attacker.example.com"}};
    auto rebind_resp = client.Get("/index.html", spoofed_headers);
    REQUIRE(rebind_resp != nullptr);
    CHECK(rebind_resp->status == 403);

    // 5. 404 Not Found for missing asset
    auto not_found_resp = client.Get("/non_existent.js");
    REQUIRE(not_found_resp != nullptr);
    CHECK(not_found_resp->status == 404);

    // 6. Directory traversal rejection
    auto traversal_resp = client.Get("/../../etc/passwd");
    REQUIRE(traversal_resp != nullptr);
    CHECK(traversal_resp->status == 403);

    server.stop();
    CHECK_FALSE(server.is_running());
}
