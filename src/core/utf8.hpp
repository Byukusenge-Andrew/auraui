#pragma once

#include "auraui/result.hpp"
#include <string>
#include <string_view>

#if defined(_WIN32)
#include <string>
#endif

namespace auraui {
namespace core {

class Utf8 {
public:
    [[nodiscard]] static bool is_valid(std::string_view str) noexcept;
    [[nodiscard]] static std::string sanitize(std::string_view str);

#if defined(_WIN32)
    [[nodiscard]] static std::wstring to_wide(std::string_view utf8);
    [[nodiscard]] static std::string to_utf8(std::wstring_view wide);
#endif
};

} // namespace core
} // namespace auraui
