#pragma once

#include "nlohmann/json.hpp"
#include <string>
#include <functional>
#include <memory>
#include <vector>
#include <mutex>
#include <unordered_map>

namespace auraui {

using EventHandler = std::function<void(const nlohmann::json&)>;
using UnsubscribeFn = std::function<void()>;

class EventBus {
private:
    mutable std::mutex m_mutex;
    std::unordered_map<std::string, std::vector<EventHandler>> m_listeners;

public:
    EventBus() = default;

    UnsubscribeFn on(const std::string& event, EventHandler handler) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_listeners[event].push_back(handler);
        return [this, event, handler]() {
            this->off(event, handler);
        };
    }

    void off(const std::string& event, const EventHandler& /*handler*/) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_listeners.erase(event);
    }

    void emit(const std::string& event, const nlohmann::json& payload) {
        std::vector<EventHandler> callbacks;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            auto it = m_listeners.find(event);
            if (it != m_listeners.end()) {
                callbacks = it->second;
            }
        }
        for (const auto& cb : callbacks) {
            try {
                cb(payload);
            } catch (...) {}
        }
    }
};

} // namespace auraui
