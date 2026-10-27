#include <doctest/doctest.h>
#include "core/store.hpp"

using namespace auraui::core;

TEST_CASE("Store - Set, get, and remove") {
    Store store("com.auraui.unittest");

    // Clear previous if any
    auto all_res = store.get_all();
    if (all_res) {
        for (auto it = all_res->begin(); it != all_res->end(); ++it) {
            (void)store.remove(it.key());
        }
    }

    CHECK(store.set("theme", "dark").has_value());
    CHECK(store.set("volume", 85).has_value());

    auto theme_res = store.get("theme");
    REQUIRE(theme_res.has_value());
    CHECK(*theme_res == "dark");

    auto vol_res = store.get("volume");
    REQUIRE(vol_res.has_value());
    CHECK(*vol_res == 85);

    CHECK(store.remove("theme").has_value());
    CHECK_FALSE(store.get("theme").has_value());
}
