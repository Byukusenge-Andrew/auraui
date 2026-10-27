#pragma once

#include "asset_store.hpp"
#include "auraui/result.hpp"
#include "auraui/error.hpp"

#include <string>
#include <memory>
#include <thread>
#include <atomic>
#include <cstdint>

namespace httplib {
class Server;
}

namespace auraui {
namespace assets {

class AssetServer {
private:
    std::shared_ptr<AssetStore> m_store;
    std::string m_csp_header;
    std::unique_ptr<httplib::Server> m_server;
    std::thread m_thread;
    std::atomic<bool> m_running{false};
    uint16_t m_port{0};

    [[nodiscard]] bool validate_host_header(std::string_view host_header) const noexcept;

public:
    AssetServer();
    ~AssetServer();

    AssetServer(const AssetServer&) = delete;
    AssetServer& operator=(const AssetServer&) = delete;
    AssetServer(AssetServer&&) noexcept;
    AssetServer& operator=(AssetServer&&) noexcept;

    Result<void> start(std::shared_ptr<AssetStore> store, std::string csp_header);
    void stop();

    [[nodiscard]] bool is_running() const noexcept;
    [[nodiscard]] uint16_t port() const noexcept { return m_port; }
    [[nodiscard]] std::string base_url() const;
};

} // namespace assets
} // namespace auraui
