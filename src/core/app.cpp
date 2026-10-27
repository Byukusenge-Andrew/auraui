#include "auraui/app.hpp"
#include "auraui/log.hpp"
#include "config.hpp"
#include "dispatcher.hpp"
#include "thread_pool.hpp"
#include "security/capability_policy.hpp"
#include "security/url_policy.hpp"
#include "security/csp.hpp"
#include "assets/asset_store.hpp"
#include "assets/asset_server.hpp"
#include "backend/webview_backend.hpp"
#include "modules/builtin_modules.hpp"

#include <iostream>

namespace auraui {

class App::Impl {
public:
    AppOptions options;
    core::AppConfig config;
    security::CapabilityPolicy policy;
    std::shared_ptr<core::ThreadPool> pool;
    std::unique_ptr<core::Dispatcher> dispatcher;
    EventBus event_bus;

    std::shared_ptr<assets::AssetStore> asset_store;
    std::unique_ptr<assets::AssetServer> asset_server;
    std::unique_ptr<backend::Backend> backend;
    std::string default_entry{"index.html"};

    explicit Impl(AppOptions opts)
        : options(std::move(opts)),
          pool(std::make_shared<core::ThreadPool>(4)),
          asset_store(std::make_shared<assets::AssetStore>())
    {
        // 1. Load & validate manifest (Fail closed)
        auto config_res = core::ConfigLoader::load_from_file(options.config_path);
        if (config_res) {
            config = std::move(*config_res);
            policy = security::CapabilityPolicy(config.capabilities);
            Logger::info("App", "Manifest loaded successfully: app_id=" + config.app_id);
        } else {
            // Fail closed: every capability denied
            policy = security::CapabilityPolicy(nlohmann::json::object());
            Logger::warn("App", "FAIL CLOSED: " + config_res.error().message() + ". All capabilities denied.");
            config.window_title = options.title;
            config.window_width = options.width;
            config.window_height = options.height;
        }

        // Configure asset root directory if provided, or fallback to standard folders
        if (!options.asset_path.empty()) {
            asset_store->set_root_directory(options.asset_path);
        } else {
            std::error_code ec;
            if (std::filesystem::exists("ui", ec)) {
                asset_store->set_root_directory("ui");
            } else if (std::filesystem::exists("dist", ec)) {
                asset_store->set_root_directory("dist");
            }
        }

        // 2. Initialize dispatcher with resolved policy and rate limit
        dispatcher = std::make_unique<core::Dispatcher>(
            policy,
            static_cast<uint32_t>(config.ipc_rate_limit_per_sec),
            pool
        );
    }
};

App::App(AppOptions options)
    : m_impl(std::make_unique<Impl>(std::move(options))) {
    modules::register_all(*this, nullptr);
}

App::App(std::string title, uint32_t width, uint32_t height) {
    AppOptions opts;
    opts.title = std::move(title);
    opts.width = width;
    opts.height = height;
    m_impl = std::make_unique<Impl>(std::move(opts));
    modules::register_all(*this, nullptr);
}

App::~App() = default;

App::App(App&&) noexcept = default;
App& App::operator=(App&&) noexcept = default;

App& App::command(CommandSpec spec, CommandHandler handler) {
    auto res = m_impl->dispatcher->register_command(std::move(spec), std::move(handler));
    if (!res) {
        throw std::runtime_error("AuraUI Route Collision: " + res.error().message());
    }
    return *this;
}

App& App::command(const std::string& name, CommandHandler handler) {
    return command(CommandSpec(name), std::move(handler));
}

App& App::use(Middleware mw) {
    m_impl->dispatcher->use(std::move(mw));
    return *this;
}

App& App::mount(const std::string& prefix, const Router& router) {
    m_impl->dispatcher->mount(prefix, router);
    return *this;
}

void App::emit(const std::string& event, const nlohmann::json& payload) {
    m_impl->event_bus.emit(event, payload);
    if (m_impl->backend) {
        std::string js = "if(window.__aura_emit){window.__aura_emit(" +
                         nlohmann::json(event).dump() + "," + payload.dump() + ");}";
        m_impl->backend->eval_js(js);
    }
}

void App::register_asset(std::string path, std::string data, std::string mime) {
    m_impl->asset_store->register_embedded(std::move(path), std::move(data), std::move(mime));
}

void App::set_asset_path(std::filesystem::path path) {
    m_impl->asset_store->set_root_directory(std::move(path));
}

void App::set_title(const std::string& title) {
    m_impl->config.window_title = title;
    if (m_impl->backend) {
        m_impl->backend->set_title(title);
    }
}

void App::set_size(uint32_t width, uint32_t height) {
    m_impl->config.window_width = width;
    m_impl->config.window_height = height;
    if (m_impl->backend) {
        m_impl->backend->set_size(static_cast<int>(width), static_cast<int>(height));
    }
}

void App::set_min_size(uint32_t width, uint32_t height) {
    m_impl->config.window_min_width = width;
    m_impl->config.window_min_height = height;
}

void App::set_center() {
    // Center window hint
}

void App::navigate(const std::string& path) {
    if (m_impl->backend) {
        if (path.rfind("http://", 0) == 0 || path.rfind("https://", 0) == 0) {
            m_impl->backend->navigate(path);
        } else if (m_impl->asset_server && m_impl->asset_server->is_running()) {
            std::string p = path;
            while (!p.empty() && (p.front() == '/' || p.front() == '\\')) p.erase(p.begin());
            m_impl->backend->navigate(m_impl->asset_server->base_url() + "/" + p);
        } else {
            m_impl->backend->navigate(path);
        }
    }
}

void App::close() {
    Logger::info("App", "Application closing requested");
    if (m_impl->backend) {
        m_impl->backend->terminate();
    }
}

void App::start(std::string entry_point) {
    m_impl->default_entry = std::move(entry_point);
}

int App::run(const std::string& entry_point_arg) {
    std::string entry_point = entry_point_arg.empty() ? m_impl->default_entry : entry_point_arg;
    if (entry_point.empty()) entry_point = "index.html";

    Logger::info("App", "Starting AuraUI Application (entry: " + entry_point + ")");

    // 1. Determine target URL
    std::string target_url;
    bool is_external = (entry_point.rfind("http://", 0) == 0 || entry_point.rfind("https://", 0) == 0);

    if (is_external) {
        // External navigation requires URL policy check
        std::vector<std::string> allowed_hosts;
        if (m_impl->config.capabilities.contains("network") &&
            m_impl->config.capabilities["network"].contains("allowed_hosts") &&
            m_impl->config.capabilities["network"]["allowed_hosts"].is_array()) {
            for (const auto& h : m_impl->config.capabilities["network"]["allowed_hosts"]) {
                if (h.is_string()) allowed_hosts.push_back(h.get<std::string>());
            }
        }
        security::UrlPolicy url_policy(std::move(allowed_hosts));
        auto val_res = url_policy.validate(entry_point);
        if (!val_res) {
            Logger::error("App", "DENIAL: Navigation to external URL denied by security policy: " + entry_point);
            return 1;
        }
        target_url = entry_point;
    } else {
        // Local asset: start loopback asset server
        if (!m_impl->asset_server) {
            m_impl->asset_server = std::make_unique<assets::AssetServer>();
        }

        std::string csp = security::Csp::get_default_csp();
        auto server_res = m_impl->asset_server->start(m_impl->asset_store, csp);
        if (!server_res) {
            Logger::error("App", "Failed to start loopback asset server: " + server_res.error().message());
            return 1;
        }

        std::string path = entry_point;
        while (!path.empty() && (path.front() == '/' || path.front() == '\\')) {
            path.erase(path.begin());
        }
        target_url = m_impl->asset_server->base_url() + "/" + path;
    }

    // 2. Initialize native window backend
    backend::WindowConfig win_cfg;
    win_cfg.title = m_impl->config.window_title;
    win_cfg.width = static_cast<int>(m_impl->config.window_width);
    win_cfg.height = static_cast<int>(m_impl->config.window_height);
    win_cfg.resizable = m_impl->config.window_resizable;
    win_cfg.devtools = m_impl->options.dev_mode || m_impl->options.debug;

    m_impl->backend = std::make_unique<backend::WebviewBackend>();
    auto init_res = m_impl->backend->initialize(win_cfg);
    if (!init_res) {
        Logger::error("App", "Failed to initialize native backend: " + init_res.error().message());
        return 1;
    }

    // 3. Connect IPC bridge (zero-trust native channel)
    m_impl->backend->bind_ipc([this](std::string_view req_json, std::function<void(std::string_view)> reply) {
        m_impl->dispatcher->dispatch(req_json, [reply = std::move(reply)](std::string resp_json) {
            reply(resp_json);
        });
    });

    // 4. Navigate and run
    Logger::info("App", "Navigating window to: " + target_url);
    m_impl->backend->navigate(target_url);
    m_impl->backend->run();

    // 5. Cleanup upon window exit
    if (m_impl->asset_server) {
        m_impl->asset_server->stop();
    }

    Logger::info("App", "AuraUI Application exited gracefully");
    return 0;
}

} // namespace auraui
