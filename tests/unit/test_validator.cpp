#include <doctest/doctest.h>
#include "core/validator.hpp"

using namespace auraui::core;

TEST_CASE("Validator - Required fields & types") {
    nlohmann::json schema = {
        {"type", "object"},
        {"required", {"username", "age"}},
        {"properties", {
            {"username", {{"type", "string"}, {"minLength", 3}, {"maxLength", 20}}},
            {"age", {{"type", "integer"}, {"minimum", 0}, {"maximum", 150}}}
        }}
    };

    // Valid
    nlohmann::json valid_args = {{"username", "alice"}, {"age", 30}};
    CHECK(Validator::validate_args(valid_args, schema).has_value());

    // Missing required field
    nlohmann::json missing_age = {{"username", "alice"}};
    CHECK_FALSE(Validator::validate_args(missing_age, schema).has_value());

    // Wrong type
    nlohmann::json wrong_type = {{"username", "alice"}, {"age", "thirty"}};
    CHECK_FALSE(Validator::validate_args(wrong_type, schema).has_value());

    // Out of range
    nlohmann::json out_of_range = {{"username", "alice"}, {"age", 200}};
    CHECK_FALSE(Validator::validate_args(out_of_range, schema).has_value());

    // Additional property rejected by default
    nlohmann::json extra = {{"username", "alice"}, {"age", 30}, {"admin", true}};
    CHECK_FALSE(Validator::validate_args(extra, schema).has_value());
}

TEST_CASE("Validator - Enums") {
    nlohmann::json schema = {
        {"type", "object"},
        {"properties", {
            {"theme", {{"type", "string"}, {"enum", {"light", "dark", "system"}}}}
        }}
    };

    CHECK(Validator::validate_args({{"theme", "dark"}}, schema).has_value());
    CHECK_FALSE(Validator::validate_args({{"theme", "neon-pink"}}, schema).has_value());
}
