#pragma once

#include "auraui/result.hpp"
#include "auraui/error.hpp"
#include "nlohmann/json.hpp"

#include <string>
#include <filesystem>
#include <mutex>

namespace auraui {
namespace core {

class Store {
private:
    std::filesystem::path m_file_path;
    mutable std::mutex m_mutex;
    static constexpr size_t MAX_STORE_SIZE_BYTES = 10485760; // 10 MiB cap

public:
    explicit Store(std::string app_id);

    [[nodiscard]] Result<nlohmann::json> get(const std::string& key) const;
    [[nodiscard]] Result<nlohmann::json> get_all() const;
    [[nodiscard]] Result<void> set(const std::string& key, nlohmann::json value);
    [[nodiscard]] Result<void> remove(const std::string& key);

    [[nodiscard]] const std::filesystem::path& file_path() const noexcept { return m_file_path; }

private:
    [[nodiscard]] Result<nlohmann::json> read_file() const;
    [[nodiscard]] Result<void> write_file_atomic(const nlohmann::json& data);
};

} // namespace core
} // namespace auraui
