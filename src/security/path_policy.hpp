#pragma once

#include "auraui/result.hpp"
#include <string>
#include <string_view>
#include <filesystem>

namespace auraui {
namespace security {

class PathPolicy {
public:
    [[nodiscard]] static Result<std::filesystem::path> canonicalize(std::string_view raw_path);
    [[nodiscard]] static Result<void> validate_safe_path(std::string_view raw_path);
    [[nodiscard]] static Result<void> is_within_root(const std::filesystem::path& target, const std::filesystem::path& root);
};

} // namespace security
} // namespace auraui
