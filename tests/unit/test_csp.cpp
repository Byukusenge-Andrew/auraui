#include <doctest/doctest.h>
#include "security/csp.hpp"

using namespace auraui::security;

TEST_CASE("CSP - Strict default policy") {
    const std::string& csp = Csp::get_default_csp();
    CHECK(csp.find("default-src 'none'") != std::string::npos);
    CHECK(csp.find("script-src 'self'") != std::string::npos);
    CHECK(csp.find("style-src 'self'") != std::string::npos);
    CHECK(csp.find("frame-ancestors 'none'") != std::string::npos);

    // Assert absence of dangerous CSP directives
    CHECK(csp.find("'unsafe-inline'") == std::string::npos);
    CHECK(csp.find("'unsafe-eval'") == std::string::npos);
}

TEST_CASE("CSP - Security headers") {
    auto headers = Csp::get_security_headers();
    CHECK(headers["X-Content-Type-Options"] == "nosniff");
    CHECK(headers["Referrer-Policy"] == "no-referrer");
    CHECK(headers["X-Frame-Options"] == "DENY");
}
