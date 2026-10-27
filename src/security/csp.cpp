#include "csp.hpp"

namespace auraui {
namespace security {

const std::string& Csp::get_default_csp() {
    static const std::string s_csp(DEFAULT_CSP);
    return s_csp;
}

std::unordered_map<std::string, std::string> Csp::get_security_headers() {
    return {
        {"Content-Security-Policy", get_default_csp()},
        {"X-Content-Type-Options", "nosniff"},
        {"Referrer-Policy", "no-referrer"},
        {"X-Frame-Options", "DENY"}
    };
}

} // namespace security
} // namespace auraui
