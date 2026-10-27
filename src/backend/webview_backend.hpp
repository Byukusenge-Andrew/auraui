#pragma once

#include "backend.hpp"
#include <memory>
#include <mutex>
#include <string>

namespace auraui {
namespace backend {

class WebviewBackend : public Backend {
private:
    void* m_webview{nullptr};
    IpcHandler m_ipc_handler;
    std::mutex m_mutex;

    static void on_ipc_call(const char* id, const char* req, void* arg);

public:
    WebviewBackend();
    ~WebviewBackend() override;

    WebviewBackend(const WebviewBackend&) = delete;
    WebviewBackend& operator=(const WebviewBackend&) = delete;

    Result<void> initialize(const WindowConfig& config) override;
    void set_title(std::string_view title) override;
    void set_size(int width, int height) override;
    void navigate(std::string_view url) override;
    void bind_ipc(IpcHandler handler) override;
    void eval_js(std::string_view js) override;
    void run() override;
    void terminate() override;
    void* native_handle() override;
};

} // namespace backend
} // namespace auraui
