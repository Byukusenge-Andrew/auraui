#include <doctest/doctest.h>
#include "security/url_policy.hpp"

using namespace auraui;
using namespace auraui::security;

TEST_CASE("UrlPolicy - Allowed URLs") {
    UrlPolicy policy;
    CHECK(policy.validate("https://auraui.dev/docs").has_value());
    CHECK(policy.validate("http://localhost:8080/api").has_value());
    CHECK(policy.validate("mailto:support@auraui.dev").has_value());
}

TEST_CASE("UrlPolicy - Table of malicious URLs") {
    UrlPolicy policy;

    // Disallowed schemes
    CHECK_FALSE(policy.validate("file:///etc/passwd").has_value());
    CHECK_FALSE(policy.validate("file:///C:/Windows/explorer.exe").has_value());
    CHECK_FALSE(policy.validate("javascript:alert(1)").has_value());
    CHECK_FALSE(policy.validate("data:text/html,<h1>PWNED</h1>").has_value());
    CHECK_FALSE(policy.validate("vbscript:msgbox(1)").has_value());

    // Embedded credentials
    CHECK_FALSE(policy.validate("http://admin:secret@malicious.com/").has_value());

    // Control characters
    std::string ctrl_url = "https://example.com/\x01\x02test";
    CHECK_FALSE(policy.validate(ctrl_url).has_value());

    // Length limit > 2048
    std::string long_url = "https://example.com/" + std::string(2040, 'x');
    CHECK_FALSE(policy.validate(long_url).has_value());
}

TEST_CASE("UrlPolicy - Host allowlist") {
    UrlPolicy policy({"auraui.dev", "*.github.com"});

    CHECK(policy.validate("https://auraui.dev/guide").has_value());
    CHECK(policy.validate("https://api.github.com/repos").has_value());
    CHECK_FALSE(policy.validate("https://evil-site.com/").has_value());
}
