#pragma once

#include "nlohmann/json.hpp"
#include <string>
#include <string_view>

namespace auraui {

enum class ErrorCode {
    invalid_argument,
    permission_denied,
    not_found,
    rate_limited,
    unsupported,
    cancelled,
    internal
};

inline std::string_view to_string(ErrorCode code) noexcept {
    switch (code) {
        case ErrorCode::invalid_argument:  return "invalid_argument";
        case ErrorCode::permission_denied: return "permission_denied";
        case ErrorCode::not_found:         return "not_found";
        case ErrorCode::rate_limited:      return "rate_limited";
        case ErrorCode::unsupported:       return "unsupported";
        case ErrorCode::cancelled:         return "cancelled";
        case ErrorCode::internal:          return "internal";
        default:                           return "internal";
    }
}

class Error {
private:
    ErrorCode m_code{ErrorCode::internal};
    std::string m_message;
    nlohmann::json m_details;

public:
    Error() = default;

    Error(ErrorCode code, std::string message, nlohmann::json details = nlohmann::json::object())
        : m_code(code), m_message(std::move(message)), m_details(std::move(details)) {}

    [[nodiscard]] ErrorCode code() const noexcept { return m_code; }
    [[nodiscard]] std::string_view code_string() const noexcept { return to_string(m_code); }
    [[nodiscard]] const std::string& message() const noexcept { return m_message; }
    [[nodiscard]] const nlohmann::json& details() const noexcept { return m_details; }

    [[nodiscard]] nlohmann::json to_json() const {
        nlohmann::json j{
            {"code", code_string()},
            {"message", m_message}
        };
        if (!m_details.empty()) {
            j["details"] = m_details;
        }
        return j;
    }
};

} // namespace auraui
