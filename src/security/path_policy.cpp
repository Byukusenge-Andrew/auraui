#include "path_policy.hpp"
#include <algorithm>

namespace auraui {
namespace security {

Result<void> PathPolicy::validate_safe_path(std::string_view raw_path) {
    if (raw_path.empty()) {
        return make_error(ErrorCode::invalid_argument, "Path is empty");
    }

    // Reject UNC and network paths
    if (raw_path.starts_with("\\\\") || raw_path.starts_with("//")) {
        return make_error(ErrorCode::permission_denied, "UNC and network share paths are blocked");
    }

    // Reject Windows device and extended-length prefixes
    if (raw_path.find("\\\\?\\") != std::string_view::npos ||
        raw_path.find("\\\\.\\") != std::string_view::npos ||
        raw_path.find("\\??\\") != std::string_view::npos) {
        return make_error(ErrorCode::permission_denied, "Device namespace and extended-length prefixes are blocked");
    }

    // Check for trailing dots or spaces on any path component (Windows bypass vulnerability)
    size_t comp_start = 0;
    for (size_t i = 0; i <= raw_path.size(); ++i) {
        if (i == raw_path.size() || raw_path[i] == '/' || raw_path[i] == '\\') {
            if (i > comp_start) {
                char last = raw_path[i - 1];
                if (last == ' ' || last == '.') {
                    // Check if component is not just "." or ".."
                    std::string_view comp = raw_path.substr(comp_start, i - comp_start);
                    if (comp != "." && comp != "..") {
                        return make_error(ErrorCode::invalid_argument, "Path components with trailing dots or spaces are rejected");
                    }
                }
            }
            comp_start = i + 1;
        }
    }

    // Reject NTFS alternate data streams (e.g. file.txt:stream:$DATA)
#if defined(_WIN32)
    // On Windows, the colon at index 1 is allowed for drive letter (e.g. C:)
    size_t colon = raw_path.find(':', 2);
    if (colon != std::string_view::npos) {
        return make_error(ErrorCode::permission_denied, "Alternate data streams (ADS) are blocked");
    }
#else
    if (raw_path.find(':') != std::string_view::npos) {
        return make_error(ErrorCode::permission_denied, "Colons in path are blocked");
    }
#endif

    return {};
}

Result<std::filesystem::path> PathPolicy::canonicalize(std::string_view raw_path) {
    auto val = validate_safe_path(raw_path);
    if (!val) {
        return tl::make_unexpected(val.error());
    }

    std::error_code ec;
    std::filesystem::path p(raw_path);

    std::filesystem::path canon = std::filesystem::weakly_canonical(p, ec);
    if (ec) {
        return make_error(ErrorCode::not_found, "Failed to resolve canonical path: " + ec.message());
    }

    return canon;
}

Result<void> PathPolicy::is_within_root(const std::filesystem::path& target, const std::filesystem::path& root) {
    std::error_code ec;
    std::filesystem::path canon_target = std::filesystem::weakly_canonical(target, ec);
    if (ec) return make_error(ErrorCode::not_found, "Target path cannot be resolved");

    std::filesystem::path canon_root = std::filesystem::weakly_canonical(root, ec);
    if (ec) return make_error(ErrorCode::not_found, "Root path cannot be resolved");

    auto target_str = canon_target.lexically_normal().string();
    auto root_str = canon_root.lexically_normal().string();

#if defined(_WIN32)
    // Case-insensitive comparison on Windows
    std::string t_lower = target_str;
    std::string r_lower = root_str;
    std::transform(t_lower.begin(), t_lower.end(), t_lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    std::transform(r_lower.begin(), r_lower.end(), r_lower.begin(), [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (!t_lower.starts_with(r_lower)) {
        return make_error(ErrorCode::permission_denied, "Path traversal escape: target is outside granted root directory");
    }
#else
    if (!target_str.starts_with(root_str)) {
        return make_error(ErrorCode::permission_denied, "Path traversal escape: target is outside granted root directory");
    }
#endif

    return {};
}

} // namespace security
} // namespace auraui
