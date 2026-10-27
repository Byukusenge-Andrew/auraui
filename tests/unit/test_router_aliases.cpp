#include <doctest/doctest.h>
#include "auraui/router.hpp"

using namespace auraui;

TEST_CASE("Router - Path normalization and subrouters") {
    Router router;
    router.command("/api/users/list/", [](Context, Reply r) { r.send({}); });
    router.command("api.ping", [](Context, Reply r) { r.send({}); });

    const auto& routes = router.routes();
    REQUIRE(routes.size() == 2);
    CHECK(routes[0].path == "api.users.list");
    CHECK(routes[1].path == "api.ping");

    Router sub;
    sub.command("/status", [](Context, Reply r) { r.send({}); });
    router.mount("/v1/admin/", sub);

    const auto& subs = router.subrouters();
    REQUIRE(subs.size() == 1);
    CHECK(subs[0].first == "v1.admin");
    REQUIRE(subs[0].second.routes().size() == 1);
    CHECK(subs[0].second.routes()[0].path == "status");
}
