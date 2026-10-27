#include "asset_store.hpp"
#include "mime.hpp"
#include "auraui/log.hpp"

#include <fstream>
#include <sstream>
#include <iomanip>

namespace auraui {
namespace assets {

AssetStore::AssetStore(std::filesystem::path root_dir)
    : m_root_dir(std::move(root_dir)) {}

void AssetStore::set_root_directory(std::filesystem::path root_dir) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    m_root_dir = std::move(root_dir);
}

const std::filesystem::path& AssetStore::root_directory() const noexcept {
    return m_root_dir;
}

std::string AssetStore::compute_etag(std::string_view content) noexcept {
    // 64-bit FNV-1a hash
    uint64_t hash = 14695981039346656037ULL;
    for (char c : content) {
        hash ^= static_cast<uint8_t>(c);
        hash *= 1099511628211ULL;
    }

    std::ostringstream oss;
    oss << "\"w/" << std::hex << hash << "-" << content.size() << "\"";
    return oss.str();
}

std::string AssetStore::normalize_asset_path(std::string_view path) {
    // Strip query string and fragment if present
    size_t q = path.find_first_of("?#");
    if (q != std::string_view::npos) {
        path = path.substr(0, q);
    }

    // Strip leading slashes
    while (!path.empty() && (path.front() == '/' || path.front() == '\\')) {
        path.remove_prefix(1);
    }

    if (path.empty()) {
        return "index.html";
    }

    // Replace backslashes with forward slashes
    std::string normalized(path);
    for (char& c : normalized) {
        if (c == '\\') c = '/';
    }

    // If path ends with slash, append index.html
    if (normalized.back() == '/') {
        normalized += "index.html";
    }

    return normalized;
}

void AssetStore::register_embedded(std::string path, std::string data, std::string mime) {
    std::unique_lock<std::shared_mutex> lock(m_mutex);
    std::string norm = normalize_asset_path(path);
    if (mime.empty()) {
        mime = Mime::from_path(norm);
    }
    std::string etag = compute_etag(data);
    m_embedded_assets[norm] = Asset{std::move(data), std::move(mime), std::move(etag)};
}

bool AssetStore::has_embedded(std::string_view path) const {
    std::shared_lock<std::shared_mutex> lock(m_mutex);
    return m_embedded_assets.find(normalize_asset_path(path)) != m_embedded_assets.end();
}

Result<Asset> AssetStore::load(std::string_view raw_path) const {
    // 1. Path traversal security check
    if (raw_path.find('\0') != std::string_view::npos) {
        return make_error(ErrorCode::invalid_argument, "Path contains null byte");
    }

    std::string norm = normalize_asset_path(raw_path);

    // Prevent directory traversal sequences
    if (norm.find("..") != std::string::npos) {
        return make_error(ErrorCode::permission_denied, "Directory traversal sequence '..' detected");
    }

    // 2. Check embedded assets first
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        auto it = m_embedded_assets.find(norm);
        if (it != m_embedded_assets.end()) {
            return it->second;
        }
    }

    // 3. Fallback to filesystem if root_dir is configured
    std::filesystem::path root;
    {
        std::shared_lock<std::shared_mutex> lock(m_mutex);
        root = m_root_dir;
    }

    if (root.empty()) {
        return make_error(ErrorCode::not_found, "Asset not found: " + norm);
    }

    std::error_code ec;
    std::filesystem::path full_path = root / norm;

    if (!std::filesystem::exists(full_path, ec) || std::filesystem::is_directory(full_path, ec)) {
        return make_error(ErrorCode::not_found, "Asset not found on disk: " + norm);
    }

    // Verify canonical path does not escape root
    std::filesystem::path canonical_root = std::filesystem::weakly_canonical(root, ec);
    std::filesystem::path canonical_file = std::filesystem::weakly_canonical(full_path, ec);

    std::string root_str = canonical_root.string();
    std::string file_str = canonical_file.string();

    if (file_str.rfind(root_str, 0) != 0) {
        Logger::warn("Security", "Asset path escape attempt detected: " + file_str);
        return make_error(ErrorCode::permission_denied, "Asset path escapes root directory");
    }

    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open()) {
        return make_error(ErrorCode::not_found, "Could not open asset file: " + norm);
    }

    std::string data((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string mime = Mime::from_path(norm);
    std::string etag = compute_etag(data);

    return Asset{std::move(data), std::move(mime), std::move(etag)};
}

} // namespace assets
} // namespace auraui
