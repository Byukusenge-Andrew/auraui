#include <doctest/doctest.h>
#include "platform/platform.hpp"
#include "modules/builtin_modules.hpp"
#include "auraui/app.hpp"
#include <filesystem>

using namespace auraui;
using namespace auraui::platform;

TEST_CASE("Platform - System Information") {
    auto info = Platform::get_system_info();
    CHECK(info.os_name == "Windows");
    CHECK_FALSE(info.os_version.empty());
    CHECK(info.architecture == "x86_64");
    CHECK(info.cpu_count > 0);
    CHECK(info.total_memory_bytes > 0);
    CHECK(info.available_memory_bytes > 0);
}

TEST_CASE("Platform - Standard Directories") {
    auto temp = Platform::temp_dir();
    CHECK_FALSE(temp.empty());
    CHECK(std::filesystem::exists(temp));

    auto app_data = Platform::app_data_dir("auraui_test");
    CHECK_FALSE(app_data.empty());

    auto cache = Platform::cache_dir("auraui_test");
    CHECK_FALSE(cache.empty());

    auto home = Platform::home_dir();
    CHECK_FALSE(home.empty());
}

TEST_CASE("Platform - Clipboard text read and write") {
    std::string test_message = "AuraUI 2.0 Native Clipboard Test Payload 42";
    auto write_res = Platform::clipboard_write_text(test_message);
    REQUIRE(write_res.has_value());

    auto read_res = Platform::clipboard_read_text();
    REQUIRE(read_res.has_value());
    CHECK(*read_res == test_message);
}
