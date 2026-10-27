#pragma once

#include "auraui/result.hpp"
#include <string>
#include <string_view>
#include <vector>

namespace auraui {
namespace security {

struct ParsedUrl {
    std::string scheme;
    std::string host;
    int port{0};
    std::string path;
    std::string query;
    std::string fragment;
};

class UrlPolicy {
private:
    std::vector<std::string> m_allowed_hosts;

public:
    UrlPolicy() = default;

    explicit UrlPolicy(std::vector<std::string> allowed_hosts)
        : m_allowed_hosts(std::move(allowed_hosts)) {}

    [[nodiscard]] static Result<ParsedUrl> parse(std::string_view url_str);
    [[nodiscard]] Result<void> validate(std::string_view url_str) const;
};

} // namespace security
} // namespace auraui
