#pragma once

#include "webui.hpp"
#include "nlohmann/json.hpp"

#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <iostream>
#include <chrono>

namespace auraui {

using json = nlohmann::json;

// ----------------------------------------------------------------------------
// Request (Req) - Express-style incoming request object
// ----------------------------------------------------------------------------
class Request {
private:
    std::string m_path;
    json m_body;
    json m_locals;
    webui::window::event* m_event{nullptr};
    std::chrono::steady_clock::time_point m_start_time;

public:
    Request(webui::window::event* e = nullptr)
        : Request("", e) {}

    Request(const std::string& path, webui::window::event* e)
        : m_path(path), m_event(e), m_start_time(std::chrono::steady_clock::now())
    {
        m_locals = json::object();
        if (!e) {
            m_body = json::object();
            return;
        }

        size_t count = e->get_count();
        if (count == 0) {
            m_body = json::object();
        } else if (count == 1) {
            std::string arg = e->get_string(0);
            try {
                m_body = json::parse(arg);
            } catch (...) {
                m_body = arg;
            }
        } else {
            m_body = json::array();
            for (size_t i = 0; i < count; ++i) {
                std::string arg = e->get_string(i);
                try {
                    m_body.push_back(json::parse(arg));
                } catch (...) {
                    m_body.push_back(arg);
                }
            }
        }
    }

    const std::string& path() const { return m_path; }
    const json& data() const { return m_body; }
    const json& body() const { return m_body; }

    // Access payload directly via subscript: req["param"]
    json operator[](const std::string& key) const {
        if (m_body.is_object() && m_body.contains(key)) {
            return m_body[key];
        }
        return json(nullptr);
    }

    bool has(const std::string& key) const {
        return m_body.is_object() && m_body.contains(key);
    }

    template <typename T>
    T get(const std::string& key, const T& default_val = T{}) const {
        if (m_body.is_object() && m_body.contains(key)) {
            try {
                return m_body[key].get<T>();
            } catch (...) {
                return default_val;
            }
        }
        return default_val;
    }

    // Context storage for middleware state passing (e.g. auth user, session)
    void set(const std::string& key, const json& val) {
        m_locals[key] = val;
    }

    json get_local(const std::string& key) const {
        if (m_locals.contains(key)) return m_locals[key];
        return json(nullptr);
    }

    template <typename T>
    T get_local(const std::string& key, const T& default_val = T{}) const {
        if (m_locals.contains(key)) {
            try {
                return m_locals[key].get<T>();
            } catch (...) {}
        }
        return default_val;
    }

    // High-resolution timing metrics
    int64_t elapsed_us() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now - m_start_time).count();
    }

    double elapsed_ms() const {
        return (double)elapsed_us() / 1000.0;
    }

    webui::window::event* raw_event() const { return m_event; }
    webui::window::event* get_event() const { return m_event; }
};

// Aliases for modern Express syntax and backward compatibility
using Req = Request;
using Context = Request;

// ----------------------------------------------------------------------------
// Response (Res) - Express-style fluent response builder
// ----------------------------------------------------------------------------
class Response {
private:
    int m_status{200};
    auraui::json m_body{nullptr};
    bool m_sent{false};

public:
    Response() = default;

    Response& status(int code) {
        m_status = code;
        return *this;
    }

    int get_status() const { return m_status; }
    bool is_sent() const { return m_sent; }

    Response& json(const auraui::json& payload) {
        m_body = payload;
        m_sent = true;
        return *this;
    }

    Response& send(const auraui::json& payload) {
        return json(payload);
    }

    Response& text(const std::string& message) {
        m_body = message;
        m_sent = true;
        return *this;
    }

    Response& ok(const auraui::json& data = auraui::json::object()) {
        m_status = 200;
        if (data.is_object()) {
            auraui::json res_obj = data;
            if (!res_obj.contains("status")) res_obj["status"] = "success";
            if (!res_obj.contains("success")) res_obj["success"] = true;
            m_body = res_obj;
        } else {
            m_body = auraui::json{
                {"success", true},
                {"status", 200},
                {"data", data}
            };
        }
        m_sent = true;
        return *this;
    }

    Response& error(const std::string& message, int code = 400, const auraui::json& details = auraui::json::object()) {
        m_status = code;
        m_body = auraui::json{
            {"success", false},
            {"status", code},
            {"error", message}
        };
        if (!details.empty()) {
            m_body["details"] = details;
        }
        m_sent = true;
        return *this;
    }

    auraui::json to_json() const {
        if (!m_sent) {
            return auraui::json{{"success", true}, {"status", m_status}};
        }
        return m_body;
    }
};

using Res = Response;

// ----------------------------------------------------------------------------
// Middleware Pipeline & Execution Model
// ----------------------------------------------------------------------------
using Next = std::function<void()>;
using Middleware = std::function<void(Req&, Res&, Next)>;
using RouteHandler = std::function<void(Req&, Res&)>;
using SimpleHandler = std::function<json(Req&)>;

struct PipelineExecution : std::enable_shared_from_this<PipelineExecution> {
    std::vector<Middleware> middlewares;
    RouteHandler final_handler;
    size_t index{0};

    PipelineExecution(std::vector<Middleware> mws, RouteHandler handler)
        : middlewares(std::move(mws)), final_handler(std::move(handler)) {}

    void next(Req& req, Res& res) {
        if (res.is_sent()) return;
        if (index < middlewares.size()) {
            size_t cur = index++;
            auto self = shared_from_this();
            try {
                middlewares[cur](req, res, [self, &req, &res]() {
                    self->next(req, res);
                });
            } catch (const std::exception& ex) {
                res.error(std::string("Middleware error: ") + ex.what(), 500);
            } catch (...) {
                res.error("Unknown middleware error", 500);
            }
        } else {
            if (!res.is_sent()) {
                try {
                    final_handler(req, res);
                    if (!res.is_sent()) {
                        res.ok();
                    }
                } catch (const std::exception& ex) {
                    res.error(std::string("Handler error: ") + ex.what(), 500);
                } catch (...) {
                    res.error("Unknown handler error", 500);
                }
            }
        }
    }
};

// ----------------------------------------------------------------------------
// Router - Modular route grouping and sub-routing
// ----------------------------------------------------------------------------
class Router {
public:
    struct RouteEntry {
        std::string path;
        RouteHandler handler;
        std::vector<Middleware> middlewares;
    };

private:
    std::vector<Middleware> m_middlewares;
    std::vector<RouteEntry> m_routes;
    std::vector<std::pair<std::string, Router>> m_subrouters;

public:
    Router() = default;

    Router& use(Middleware mw) {
        m_middlewares.push_back(std::move(mw));
        return *this;
    }

    Router& use(const std::string& prefix, const Router& subrouter) {
        m_subrouters.emplace_back(prefix, subrouter);
        return *this;
    }

    Router& route(const std::string& path, RouteHandler handler) {
        m_routes.push_back({path, std::move(handler), {}});
        return *this;
    }

    Router& route(const std::string& path, SimpleHandler handler) {
        return route(path, [handler](Req& req, Res& res) {
            json out = handler(req);
            res.ok(out);
        });
    }

    Router& route(const std::string& path, Middleware mw, RouteHandler handler) {
        m_routes.push_back({path, std::move(handler), {std::move(mw)}});
        return *this;
    }

    Router& on(const std::string& path, RouteHandler handler) {
        return route(path, std::move(handler));
    }

    Router& on(const std::string& path, SimpleHandler handler) {
        return route(path, std::move(handler));
    }

    const std::vector<Middleware>& middlewares() const { return m_middlewares; }
    const std::vector<RouteEntry>& routes() const { return m_routes; }
    const std::vector<std::pair<std::string, Router>>& subrouters() const { return m_subrouters; }
};

// ----------------------------------------------------------------------------
// Built-in Middleware Suite
// ----------------------------------------------------------------------------
namespace middleware {

inline Middleware logger() {
    return [](Req& req, Res& res, Next next) {
        auto t0 = std::chrono::steady_clock::now();
        next();
        auto t1 = std::chrono::steady_clock::now();
        auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();

        int status = res.get_status();
        const char* color_code = (status >= 400) ? "\033[31m" : (status >= 300) ? "\033[33m" : "\033[32m";
        const char* reset_code = "\033[0m";

        std::cout << "[AuraUI] " << color_code << status << reset_code 
                  << " " << req.path() 
                  << " (" << elapsed_us << " \xC2\xB5s)" << std::endl;
    };
}

inline Middleware timing() {
    return [](Req& req, Res& res, Next next) {
        next();
        json j = res.to_json();
        if (j.is_object() && !j.contains("_time_us")) {
            j["_time_us"] = req.elapsed_us();
            res.json(j);
        }
    };
}

inline Middleware guard(std::function<bool(const Req&)> validator, const std::string& err_msg = "Access Forbidden") {
    return [validator, err_msg](Req& req, Res& res, Next next) {
        if (validator(req)) {
            next();
        } else {
            res.error(err_msg, 403);
        }
    };
}

} // namespace middleware

} // namespace auraui
