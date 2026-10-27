#pragma once

#include <string>
#include <string_view>

namespace auraui {
namespace assets {

class Mime {
public:
    [[nodiscard]] static std::string from_extension(std::string_view ext) noexcept;
    [[nodiscard]] static std::string from_path(std::string_view path) noexcept;
};

} // namespace assets
} // namespace auraui
