#pragma once

#include "command.hpp"
#include "middleware.hpp"

#include <string>
#include <vector>
#include <unordered_set>
#include <utility>
#include <stdexcept>
#include <algorithm>

namespace auraui {

class Router {
public:
    struct RouteEntry {
        std::string path;
        CommandSpec spec;
        CommandHandler handler;
        std::vector<Middleware> middlewares;
    };

private:
    std::vector<Middleware> m_middlewares;
    std::vector<RouteEntry> m_routes;
    std::vector<std::pair<std::string, Router>> m_subrouters;

    static std::string normalize_path(const std::string& path) {
        size_t start = 0;
        while (start < path.size() && (path[start] == '/' || path[start] == '.')) ++start;
        size_t end = path.size();
        while (end > start && (path[end - 1] == '/' || path[end - 1] == '.')) --end;
        std::string clean = path.substr(start, end - start);
        std::replace(clean.begin(), clean.end(), '/', '.');
        return clean;
    }

public:
    Router() = default;

    Router& use(Middleware mw) {
        m_middlewares.push_back(std::move(mw));
        return *this;
    }

    Router& mount(const std::string& prefix, const Router& subrouter) {
        m_subrouters.emplace_back(normalize_path(prefix), subrouter);
        return *this;
    }

    Router& command(CommandSpec spec, CommandHandler handler) {
        spec.name = normalize_path(spec.name);
        m_routes.push_back({spec.name, spec, std::move(handler), {}});
        return *this;
    }

    Router& command(const std::string& name, CommandHandler handler) {
        return command(CommandSpec(name), std::move(handler));
    }

    Router& command(const std::string& name, Middleware mw, CommandHandler handler) {
        CommandSpec spec(normalize_path(name));
        m_routes.push_back({spec.name, spec, std::move(handler), {std::move(mw)}});
        return *this;
    }

    // Express-style route() aliases
    Router& route(const std::string& name, CommandHandler handler) {
        return command(name, std::move(handler));
    }

    Router& on(const std::string& name, CommandHandler handler) {
        return command(name, std::move(handler));
    }

    [[nodiscard]] const std::vector<Middleware>& middlewares() const noexcept { return m_middlewares; }
    [[nodiscard]] const std::vector<RouteEntry>& routes() const noexcept { return m_routes; }
    [[nodiscard]] const std::vector<std::pair<std::string, Router>>& subrouters() const noexcept { return m_subrouters; }
};

} // namespace auraui
