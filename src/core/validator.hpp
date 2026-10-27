#pragma once

#include "auraui/result.hpp"
#include "auraui/error.hpp"
#include "nlohmann/json.hpp"

namespace auraui {
namespace core {

class Validator {
public:
    [[nodiscard]] static Result<void> validate_args(const nlohmann::json& args, const nlohmann::json& schema);
};

} // namespace core
} // namespace auraui
