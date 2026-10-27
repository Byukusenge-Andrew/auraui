#include "validator.hpp"

namespace auraui {
namespace core {

Result<void> Validator::validate_args(const nlohmann::json& args, const nlohmann::json& schema) {
    if (schema.empty() || !schema.is_object()) {
        return {}; // No schema defined -> pass
    }

    // Check required type
    if (schema.contains("type") && schema["type"].is_string()) {
        std::string expected_type = schema["type"].get<std::string>();
        if (expected_type == "object" && !args.is_object()) {
            return make_error(ErrorCode::invalid_argument, "Arguments must be an object");
        }
        if (expected_type == "array" && !args.is_array()) {
            return make_error(ErrorCode::invalid_argument, "Arguments must be an array");
        }
        if (expected_type == "string" && !args.is_string()) {
            return make_error(ErrorCode::invalid_argument, "Arguments must be a string");
        }
    }

    if (!args.is_object()) {
        return {};
    }

    // Check required fields
    if (schema.contains("required") && schema["required"].is_array()) {
        for (const auto& req_key : schema["required"]) {
            if (req_key.is_string()) {
                std::string k = req_key.get<std::string>();
                if (!args.contains(k) || args[k].is_null()) {
                    return make_error(ErrorCode::invalid_argument, "Missing required argument property: " + k);
                }
            }
        }
    }

    // Check properties
    if (schema.contains("properties") && schema["properties"].is_object()) {
        const auto& props = schema["properties"];

        // Check additionalProperties: false by default
        bool allow_additional = false;
        if (schema.contains("additionalProperties") && schema["additionalProperties"].is_boolean()) {
            allow_additional = schema["additionalProperties"].get<bool>();
        }

        if (!allow_additional) {
            for (auto it = args.begin(); it != args.end(); ++it) {
                if (!props.contains(it.key())) {
                    return make_error(ErrorCode::invalid_argument, "Unexpected property not permitted by schema: " + it.key());
                }
            }
        }

        for (auto it = props.begin(); it != props.end(); ++it) {
            const std::string& prop_name = it.key();
            const auto& prop_spec = it.value();

            if (!args.contains(prop_name)) continue;
            const auto& val = args[prop_name];

            // Type check
            if (prop_spec.contains("type") && prop_spec["type"].is_string()) {
                std::string t = prop_spec["type"].get<std::string>();
                if (t == "string" && !val.is_string()) {
                    return make_error(ErrorCode::invalid_argument, "Property '" + prop_name + "' must be a string");
                }
                if (t == "number" && !val.is_number()) {
                    return make_error(ErrorCode::invalid_argument, "Property '" + prop_name + "' must be a number");
                }
                if (t == "integer" && !val.is_number_integer()) {
                    return make_error(ErrorCode::invalid_argument, "Property '" + prop_name + "' must be an integer");
                }
                if (t == "boolean" && !val.is_boolean()) {
                    return make_error(ErrorCode::invalid_argument, "Property '" + prop_name + "' must be a boolean");
                }
                if (t == "array" && !val.is_array()) {
                    return make_error(ErrorCode::invalid_argument, "Property '" + prop_name + "' must be an array");
                }
                if (t == "object" && !val.is_object()) {
                    return make_error(ErrorCode::invalid_argument, "Property '" + prop_name + "' must be an object");
                }
            }

            // String constraints
            if (val.is_string()) {
                std::string s = val.get<std::string>();
                if (prop_spec.contains("minLength") && prop_spec["minLength"].is_number_unsigned()) {
                    if (s.size() < prop_spec["minLength"].get<size_t>()) {
                        return make_error(ErrorCode::invalid_argument, "Property '" + prop_name + "' is shorter than minLength");
                    }
                }
                if (prop_spec.contains("maxLength") && prop_spec["maxLength"].is_number_unsigned()) {
                    if (s.size() > prop_spec["maxLength"].get<size_t>()) {
                        return make_error(ErrorCode::invalid_argument, "Property '" + prop_name + "' exceeds maxLength");
                    }
                }
            }

            // Numeric constraints
            if (val.is_number()) {
                double num = val.get<double>();
                if (prop_spec.contains("minimum") && prop_spec["minimum"].is_number()) {
                    if (num < prop_spec["minimum"].get<double>()) {
                        return make_error(ErrorCode::invalid_argument, "Property '" + prop_name + "' is less than minimum");
                    }
                }
                if (prop_spec.contains("maximum") && prop_spec["maximum"].is_number()) {
                    if (num > prop_spec["maximum"].get<double>()) {
                        return make_error(ErrorCode::invalid_argument, "Property '" + prop_name + "' exceeds maximum");
                    }
                }
            }

            // Enums
            if (prop_spec.contains("enum") && prop_spec["enum"].is_array()) {
                bool matched = false;
                for (const auto& item : prop_spec["enum"]) {
                    if (val == item) {
                        matched = true;
                        break;
                    }
                }
                if (!matched) {
                    return make_error(ErrorCode::invalid_argument, "Property '" + prop_name + "' value is not in allowed enum set");
                }
            }
        }
    }

    return {};
}

} // namespace core
} // namespace auraui
