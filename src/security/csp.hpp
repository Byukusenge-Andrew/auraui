#pragma once

#include <string>
#include <string_view>
#include <unordered_map>

namespace auraui {
namespace security {

class Csp {
public:
    static constexpr std::string_view DEFAULT_CSP =
        "default-src 'none'; "
        "script-src 'self'; "
        "style-src 'self'; "
        "img-src 'self' data:; "
        "font-src 'self'; "
        "connect-src 'self'; "
        "base-uri 'none'; "
        "form-action 'none'; "
        "frame-ancestors 'none';";

    [[nodiscard]] static const std::string& get_default_csp();
    [[nodiscard]] static std::unordered_map<std::string, std::string> get_security_headers();
};

} // namespace security
} // namespace auraui
