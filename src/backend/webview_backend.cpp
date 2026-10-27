#include "webview_backend.hpp"
#include "webview/webview.h"
#include "nlohmann/json.hpp"
#include "auraui/log.hpp"

#include <iostream>

namespace auraui {
namespace backend {

WebviewBackend::WebviewBackend() = default;

WebviewBackend::~WebviewBackend() {
    if (m_webview) {
        webview_destroy(m_webview);
        m_webview = nullptr;
    }
}

Result<void> WebviewBackend::initialize(const WindowConfig& config) {
    if (m_webview) {
        return make_error(ErrorCode::invalid_argument, "WebviewBackend is already initialized");
    }

    int debug_mode = config.devtools ? 1 : 0;
    m_webview = webview_create(debug_mode, nullptr);
    if (!m_webview) {
        return make_error(ErrorCode::internal, "Failed to create native webview window instance");
    }

    set_title(config.title);
    set_size(config.width, config.height);

    // Bind low-level IPC callback
    webview_error_t bind_err = webview_bind(
        m_webview,
        "__aura_ipc_native",
        &WebviewBackend::on_ipc_call,
        this
    );

    if (bind_err != WEBVIEW_ERROR_OK) {
        webview_destroy(m_webview);
        m_webview = nullptr;
        return make_error(ErrorCode::internal, "Failed to bind IPC native channel to webview");
    }

    // Inject frontend bridge wrapper
    const char* init_script = R"(
        (function() {
            window.__aura_ipc = function(req) {
                var payload = typeof req === 'string' ? req : JSON.stringify(req);
                return window.__aura_ipc_native(payload);
            };
        })();
    )";
    webview_init(m_webview, init_script);

    return {};
}

void WebviewBackend::set_title(std::string_view title) {
    if (m_webview) {
        std::string t(title);
        webview_set_title(m_webview, t.c_str());
    }
}

void WebviewBackend::set_size(int width, int height) {
    if (m_webview) {
        webview_set_size(m_webview, width, height, WEBVIEW_HINT_NONE);
    }
}

void WebviewBackend::navigate(std::string_view url) {
    if (m_webview) {
        std::string u(url);
        webview_navigate(m_webview, u.c_str());
    }
}

void WebviewBackend::bind_ipc(IpcHandler handler) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ipc_handler = std::move(handler);
}

void WebviewBackend::eval_js(std::string_view js) {
    if (m_webview) {
        std::string s(js);
        webview_eval(m_webview, s.c_str());
    }
}

void WebviewBackend::run() {
    if (m_webview) {
        webview_run(m_webview);
    }
}

void WebviewBackend::terminate() {
    if (m_webview) {
        webview_terminate(m_webview);
    }
}

void* WebviewBackend::native_handle() {
    if (m_webview) {
        return webview_get_window(m_webview);
    }
    return nullptr;
}

void WebviewBackend::on_ipc_call(const char* id, const char* req, void* arg) {
    auto* self = static_cast<WebviewBackend*>(arg);
    if (!self || !self->m_webview || !id) {
        return;
    }

    std::string call_id(id);
    void* w = self->m_webview;

    // req is a JSON array string containing arguments passed to JS function
    std::string payload;
    try {
        nlohmann::json args_arr = nlohmann::json::parse(req ? req : "[]");
        if (args_arr.is_array() && !args_arr.empty()) {
            if (args_arr[0].is_string()) {
                payload = args_arr[0].get<std::string>();
            } else {
                payload = args_arr[0].dump();
            }
        }
    } catch (...) {
        payload = req ? req : "";
    }

    IpcHandler handler;
    {
        std::lock_guard<std::mutex> lock(self->m_mutex);
        handler = self->m_ipc_handler;
    }

    if (!handler) {
        webview_return(w, call_id.c_str(), 1, "{\"ok\":false,\"error\":{\"code\":\"internal\",\"message\":\"No IPC handler registered\"}}");
        return;
    }

    handler(payload, [w, call_id](std::string_view resp) {
        std::string r(resp);
        webview_return(w, call_id.c_str(), 0, r.c_str());
    });
}

} // namespace backend
} // namespace auraui
