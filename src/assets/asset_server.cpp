#include "asset_server.hpp"
#include "httplib/httplib.h"
#include "auraui/log.hpp"

#include <iostream>

namespace auraui {
namespace assets {

AssetServer::AssetServer() = default;

AssetServer::~AssetServer() {
    stop();
}

AssetServer::AssetServer(AssetServer&& other) noexcept
    : m_store(std::move(other.m_store)),
      m_csp_header(std::move(other.m_csp_header)),
      m_server(std::move(other.m_server)),
      m_thread(std::move(other.m_thread)),
      m_running(other.m_running.load()),
      m_port(other.m_port) {
    other.m_running = false;
    other.m_port = 0;
}

AssetServer& AssetServer::operator=(AssetServer&& other) noexcept {
    if (this != &other) {
        stop();
        m_store = std::move(other.m_store);
        m_csp_header = std::move(other.m_csp_header);
        m_server = std::move(other.m_server);
        m_thread = std::move(other.m_thread);
        m_running = other.m_running.load();
        m_port = other.m_port;
        other.m_running = false;
        other.m_port = 0;
    }
    return *this;
}

bool AssetServer::validate_host_header(std::string_view host_header) const noexcept {
    if (host_header.empty()) {
        return false;
    }

    std::string expected_port_suffix = ":" + std::to_string(m_port);

    // Permitted: "127.0.0.1:<port>", "localhost:<port>", or bare "127.0.0.1"/"localhost"
    if (host_header == "127.0.0.1" || host_header == "localhost") {
        return true;
    }

    if (host_header.size() > expected_port_suffix.size()) {
        if (host_header.ends_with(expected_port_suffix)) {
            std::string_view prefix = host_header.substr(0, host_header.size() - expected_port_suffix.size());
            if (prefix == "127.0.0.1" || prefix == "localhost" || prefix == "[::1]") {
                return true;
            }
        }
    }

    return false;
}

Result<void> AssetServer::start(std::shared_ptr<AssetStore> store, std::string csp_header) {
    if (m_running.load()) {
        return make_error(ErrorCode::invalid_argument, "AssetServer is already running");
    }

    if (!store) {
        return make_error(ErrorCode::invalid_argument, "AssetStore must not be null");
    }

    m_store = std::move(store);
    m_csp_header = std::move(csp_header);
    m_server = std::make_unique<httplib::Server>();

    // Bind to 127.0.0.1 on an ephemeral port assigned by the OS
    int port = m_server->bind_to_any_port("127.0.0.1");
    if (port <= 0) {
        m_server.reset();
        return make_error(ErrorCode::internal, "Failed to bind loopback asset server to 127.0.0.1");
    }
    m_port = static_cast<uint16_t>(port);

    // Register catch-all GET route for assets
    m_server->Get(".*", [this](const httplib::Request& req, httplib::Response& res) {
        // 1. Host header validation (Neutralize DNS Rebinding)
        std::string host = req.get_header_value("Host");
        if (!validate_host_header(host)) {
            Logger::warn("Security", "DENIAL: DNS Rebinding attempt detected with Host header: '" + host + "'");
            res.status = 403;
            res.set_content("Forbidden: Invalid Host header", "text/plain; charset=utf-8");
            return;
        }

        // 2. Set strict security headers
        if (!m_csp_header.empty()) {
            res.set_header("Content-Security-Policy", m_csp_header);
        }
        res.set_header("X-Content-Type-Options", "nosniff");
        res.set_header("X-Frame-Options", "DENY");
        res.set_header("Referrer-Policy", "no-referrer");

        // 3. Load asset from store
        auto asset_res = m_store->load(req.path);
        if (!asset_res) {
            int code = (asset_res.error().code() == ErrorCode::permission_denied) ? 403 : 404;
            res.status = code;
            res.set_content(asset_res.error().message(), "text/plain; charset=utf-8");
            return;
        }

        const auto& asset = *asset_res;

        // 4. ETag caching
        res.set_header("ETag", asset.etag);
        res.set_header("Cache-Control", "public, max-age=0, must-revalidate");

        if (req.has_header("If-None-Match") && req.get_header_value("If-None-Match") == asset.etag) {
            res.status = 304;
            return;
        }

        // 5. Successful response
        res.status = 200;
        res.set_content(asset.data, asset.mime_type);
    });

    m_running = true;
    m_thread = std::thread([this]() {
        m_server->listen_after_bind();
        m_running = false;
    });

    m_server->wait_until_ready();
    Logger::info("AssetServer", "Loopback asset server listening at " + base_url());
    return {};
}

void AssetServer::stop() {
    if (m_server && m_server->is_running()) {
        m_server->stop();
    }
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_running = false;
    m_port = 0;
    m_server.reset();
}

bool AssetServer::is_running() const noexcept {
    return m_running.load() && m_server && m_server->is_running();
}

std::string AssetServer::base_url() const {
    if (m_port == 0) return "";
    return "http://127.0.0.1:" + std::to_string(m_port);
}

} // namespace assets
} // namespace auraui
