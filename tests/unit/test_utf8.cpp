#include <doctest/doctest.h>
#include "core/utf8.hpp"

using namespace auraui::core;

TEST_CASE("Utf8 - Valid strings") {
    CHECK(Utf8::is_valid("Hello World"));
    CHECK(Utf8::is_valid("Bonjour le monde, café"));
    CHECK(Utf8::is_valid("こんにちは世界")); // Japanese
    CHECK(Utf8::is_valid("你好，世界"));   // Chinese
    CHECK(Utf8::is_valid("🚀🌟🔥"));         // Emojis (4-byte sequences)
    CHECK(Utf8::is_valid(""));
}

TEST_CASE("Utf8 - Reject invalid sequences") {
    // Overlong 2-byte NUL
    std::string overlong_nul = "\xC0\x80";
    CHECK_FALSE(Utf8::is_valid(overlong_nul));

    // Overlong 2-byte slash
    std::string overlong_slash = "\xC0\xAF";
    CHECK_FALSE(Utf8::is_valid(overlong_slash));

    // Incomplete sequence
    std::string incomplete = "abc\xE2\x82";
    CHECK_FALSE(Utf8::is_valid(incomplete));

    // UTF-16 surrogate (0xD800 - 0xDFFF)
    std::string surrogate = "\xED\xA0\x80"; // U+D800
    CHECK_FALSE(Utf8::is_valid(surrogate));
}

TEST_CASE("Utf8 - Sanitization never throws") {
    std::string invalid_input = "bad\xC0\x80" "test" "\xED\xA0\x80" "end";
    std::string sanitized = Utf8::sanitize(invalid_input);
    CHECK(Utf8::is_valid(sanitized));
    CHECK(sanitized.find("bad") != std::string::npos);
    CHECK(sanitized.find("test") != std::string::npos);
}
