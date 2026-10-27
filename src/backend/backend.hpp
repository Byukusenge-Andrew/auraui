#pragma once

#include "auraui/result.hpp"
#include "auraui/error.hpp"

#include <string>
#include <string_view>
#include <functional>

namespace auraui {
namespace backend {

struct WindowConfig {
    std::string title{"AuraUI"};
    int width{1024};
    int height{768};
    bool resizable{true};
    bool devtools{false};
    bool frameless{false};
};

using IpcHandler = std::function<void(std::string_view req_json, std::function<void(std::string_view resp_json)> reply)>;

class Backend {
public:
    virtual ~Backend() = default;

    virtual Result<void> initialize(const WindowConfig& config) = 0;
    virtual void set_title(std::string_view title) = 0;
    virtual void set_size(int width, int height) = 0;
    virtual void navigate(std::string_view url) = 0;
    virtual void bind_ipc(IpcHandler handler) = 0;
    virtual void eval_js(std::string_view js) = 0;
    virtual void run() = 0;
    virtual void terminate() = 0;
    virtual void* native_handle() = 0;
};

} // namespace backend
} // namespace auraui
