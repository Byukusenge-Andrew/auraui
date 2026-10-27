#pragma once

#include "auraui/result.hpp"
#include "auraui/error.hpp"

#include <string>
#include <string_view>
#include <vector>
#include <unordered_map>
#include <filesystem>
#include <shared_mutex>
#include <memory>

namespace auraui {
namespace assets {

struct Asset {
    std::string data;
    std::string mime_type;
    std::string etag;
};

class AssetStore {
private:
    mutable std::shared_mutex m_mutex;
    std::filesystem::path m_root_dir;
    std::unordered_map<std::string, Asset> m_embedded_assets;

    [[nodiscard]] static std::string compute_etag(std::string_view content) noexcept;
    [[nodiscard]] static std::string normalize_asset_path(std::string_view path);

public:
    explicit AssetStore(std::filesystem::path root_dir = "");

    void set_root_directory(std::filesystem::path root_dir);
    [[nodiscard]] const std::filesystem::path& root_directory() const noexcept;

    void register_embedded(std::string path, std::string data, std::string mime = "");
    [[nodiscard]] bool has_embedded(std::string_view path) const;

    [[nodiscard]] Result<Asset> load(std::string_view path) const;
};

} // namespace assets
} // namespace auraui
