#include <doctest/doctest.h>
#include "security/path_policy.hpp"

using namespace auraui;
using namespace auraui::security;

TEST_CASE("PathPolicy - Disallowed prefixes & UNC") {
    // UNC paths
    CHECK_FALSE(PathPolicy::validate_safe_path("\\\\attacker.com\\share\\file.txt").has_value());
    CHECK_FALSE(PathPolicy::validate_safe_path("//evil.org/share/payload.exe").has_value());

    // Device namespace & extended length
    CHECK_FALSE(PathPolicy::validate_safe_path("\\\\?\\C:\\Windows\\System32").has_value());
    CHECK_FALSE(PathPolicy::validate_safe_path("\\\\.\\pipe\\mypipe").has_value());
    CHECK_FALSE(PathPolicy::validate_safe_path("\\??\\C:\\test").has_value());
}

TEST_CASE("PathPolicy - Alternate Data Streams & Trailing characters") {
    // Alternate data streams
#if defined(_WIN32)
    CHECK_FALSE(PathPolicy::validate_safe_path("C:\\test\\file.txt:hidden_stream").has_value());
#endif

    // Trailing dots and spaces
    CHECK_FALSE(PathPolicy::validate_safe_path("C:/folder/secret. ").has_value());
    CHECK_FALSE(PathPolicy::validate_safe_path("/home/user/document...").has_value());
}

TEST_CASE("PathPolicy - Root containment check") {
    namespace fs = std::filesystem;
    fs::path root = fs::current_path();
    fs::path inside = root / "docs";
    fs::path outside = root.parent_path();

    CHECK(PathPolicy::is_within_root(inside, root).has_value());
    CHECK_FALSE(PathPolicy::is_within_root(outside, root).has_value());
}
