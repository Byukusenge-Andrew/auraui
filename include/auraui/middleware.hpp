#pragma once

#include "command.hpp"
#include "error.hpp"
#include "capabilities.hpp"

#include <functional>
#include <optional>
#include <iostream>
#include <chrono>

namespace auraui {

class NextFunction {
private:
    std::function<void(std::optional<Error>)> m_fn;

public:
    NextFunction() = default;

    explicit NextFunction(std::function<void(std::optional<Error>)> fn)
        : m_fn(std::move(fn)) {}

    void operator()() const {
        if (m_fn) m_fn(std::nullopt);
    }

    void operator()(Error err) const {
        if (m_fn) m_fn(std::move(err));
    }
};

using Next = NextFunction;
using Middleware = std::function<void(Context&, Next)>;

namespace middleware {

inline Middleware logger() {
    return [](Context& ctx, Next next) {
        auto t0 = std::chrono::steady_clock::now();
        std::cout << "[AuraUI IPC] -> " << ctx.cmd() << " (id: " << ctx.id() << ")\n";
        next();
        auto t1 = std::chrono::steady_clock::now();
        auto elapsed_us = std::chrono::duration_cast<std::chrono::microseconds>(t1 - t0).count();
        std::cout << "[AuraUI IPC] <- " << ctx.cmd() << " completed (" << elapsed_us << " us)\n";
    };
}

inline Middleware timing() {
    return [](Context& ctx, Next next) {
        ctx.set_local("_start_us", ctx.elapsed_us());
        next();
    };
}

inline Middleware guard(std::function<bool(const Context&)> validator, std::string err_msg = "Access Forbidden") {
    return [validator = std::move(validator), err_msg = std::move(err_msg)](Context& ctx, Next next) {
        if (validator(ctx)) {
            next();
        } else {
            next(Error(ErrorCode::permission_denied, err_msg));
        }
    };
}

inline Middleware require_capability(std::string capability) {
    return [cap = std::move(capability)](Context& ctx, Next next) {
        auto granted = ctx.get_local("_capabilities");
        if (granted.is_array()) {
            for (const auto& item : granted) {
                if (item.is_string() && item.get<std::string>() == cap) {
                    next();
                    return;
                }
            }
        }
        next(Error(ErrorCode::permission_denied, "Missing required capability: " + cap));
    };
}

} // namespace middleware
} // namespace auraui
