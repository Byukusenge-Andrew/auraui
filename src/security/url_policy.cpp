#include "url_policy.hpp"
#include <algorithm>
#include <cctype>

namespace auraui {
namespace security {

Result<ParsedUrl> UrlPolicy::parse(std::string_view url_str) {
    if (url_str.empty()) {
        return make_error(ErrorCode::invalid_argument, "URL string is empty");
    }

    if (url_str.size() > 2048) {
        return make_error(ErrorCode::invalid_argument, "URL exceeds maximum length of 2048 characters");
    }

    // Check control characters
    for (char c : url_str) {
        unsigned char uc = static_cast<unsigned char>(c);
        if (uc < 32 || uc == 127) {
            return make_error(ErrorCode::invalid_argument, "URL contains disallowed control characters");
        }
    }

    // Parse scheme
    size_t colon_pos = url_str.find(':');
    if (colon_pos == std::string_view::npos || colon_pos == 0) {
        return make_error(ErrorCode::invalid_argument, "URL is missing a valid scheme delimiter");
    }

    std::string scheme;
    scheme.reserve(colon_pos);
    for (size_t i = 0; i < colon_pos; ++i) {
        char c = url_str[i];
        if (i == 0 && !std::isalpha(static_cast<unsigned char>(c))) {
            return make_error(ErrorCode::invalid_argument, "URL scheme must begin with an alphabetic character");
        }
        scheme.push_back(static_cast<char>(std::tolower(static_cast<unsigned char>(c))));
    }

    // Scheme allowlist
    if (scheme != "http" && scheme != "https" && scheme != "mailto") {
        return make_error(ErrorCode::permission_denied, "URL scheme '" + scheme + "' is blocked by security policy");
    }

    ParsedUrl parsed;
    parsed.scheme = scheme;

    if (scheme == "mailto") {
        std::string_view remainder = url_str.substr(colon_pos + 1);
        if (remainder.empty() || remainder.find('@') == std::string_view::npos) {
            return make_error(ErrorCode::invalid_argument, "Invalid mailto URL format");
        }
        parsed.path = std::string(remainder);
        return parsed;
    }

    // For http / https: authority must begin with //
    std::string_view remainder = url_str.substr(colon_pos + 1);
    if (!remainder.starts_with("//")) {
        return make_error(ErrorCode::invalid_argument, "HTTP/HTTPS URL must start with '//'");
    }
    remainder.remove_prefix(2);

    // Find path start
    size_t path_pos = remainder.find_first_of("/?#");
    std::string_view authority = (path_pos == std::string_view::npos) ? remainder : remainder.substr(0, path_pos);
    std::string_view path_and_rest = (path_pos == std::string_view::npos) ? "" : remainder.substr(path_pos);

    // Reject embedded credentials: user:pass@host
    if (authority.find('@') != std::string_view::npos) {
        return make_error(ErrorCode::permission_denied, "URLs with embedded credentials (@) are rejected");
    }

    // Parse host and port
    size_t port_colon = authority.find(':');
    if (port_colon != std::string_view::npos) {
        parsed.host = std::string(authority.substr(0, port_colon));
        std::string_view port_str = authority.substr(port_colon + 1);
        try {
            int p = std::stoi(std::string(port_str));
            if (p <= 0 || p > 65535) {
                return make_error(ErrorCode::invalid_argument, "Invalid port number in URL");
            }
            parsed.port = p;
        } catch (...) {
            return make_error(ErrorCode::invalid_argument, "Malformed port in URL");
        }
    } else {
        parsed.host = std::string(authority);
        parsed.port = (scheme == "https") ? 443 : 80;
    }

    if (parsed.host.empty()) {
        return make_error(ErrorCode::invalid_argument, "URL host cannot be empty");
    }

    // Convert host to lowercase
    std::transform(parsed.host.begin(), parsed.host.end(), parsed.host.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    parsed.path = std::string(path_and_rest);
    return parsed;
}

Result<void> UrlPolicy::validate(std::string_view url_str) const {
    auto res = parse(url_str);
    if (!res) {
        return tl::make_unexpected(res.error());
    }

    const ParsedUrl& url = *res;

    // Optional per-app host allowlist check
    if (!m_allowed_hosts.empty() && url.scheme != "mailto") {
        bool host_allowed = false;
        for (const auto& allowed : m_allowed_hosts) {
            if (url.host == allowed) {
                host_allowed = true;
                break;
            }
            // Support wildcard domain matching *.example.com
            if (allowed.starts_with("*.") && url.host.ends_with(allowed.substr(1))) {
                host_allowed = true;
                break;
            }
        }
        if (!host_allowed) {
            return make_error(ErrorCode::permission_denied, "Host '" + url.host + "' is not permitted by manifest allowlist");
        }
    }

    return {};
}

} // namespace security
} // namespace auraui
