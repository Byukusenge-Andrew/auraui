#pragma once

#include "result.hpp"
#include "error.hpp"
#include "nlohmann/json.hpp"

#include <string>
#include <string_view>
#include <functional>
#include <chrono>
#include <atomic>
#include <memory>
#include <utility>

namespace auraui {

class Context {
private:
    std::string m_cmd;
    std::string m_id;
    nlohmann::json m_args;
    nlohmann::json m_locals;
    std::chrono::steady_clock::time_point m_start_time;

public:
    Context(std::string cmd, std::string id, nlohmann::json args)
        : m_cmd(std::move(cmd)),
          m_id(std::move(id)),
          m_args(std::move(args)),
          m_locals(nlohmann::json::object()),
          m_start_time(std::chrono::steady_clock::now()) {}

    [[nodiscard]] const std::string& cmd() const noexcept { return m_cmd; }
    [[nodiscard]] const std::string& id() const noexcept { return m_id; }
    [[nodiscard]] const nlohmann::json& args() const noexcept { return m_args; }
    [[nodiscard]] const nlohmann::json& body() const noexcept { return m_args; }

    [[nodiscard]] nlohmann::json operator[](const std::string& key) const {
        if (m_args.is_object() && m_args.contains(key)) {
            return m_args[key];
        }
        return nlohmann::json(nullptr);
    }

    [[nodiscard]] bool has(const std::string& key) const {
        return m_args.is_object() && m_args.contains(key);
    }

    template <typename T>
    [[nodiscard]] T get(const std::string& key, const T& default_val = T{}) const {
        if (m_args.is_object() && m_args.contains(key)) {
            try {
                return m_args[key].get<T>();
            } catch (...) {
                return default_val;
            }
        }
        return default_val;
    }

    void set_local(const std::string& key, const nlohmann::json& value) {
        m_locals[key] = value;
    }

    [[nodiscard]] nlohmann::json get_local(const std::string& key) const {
        if (m_locals.contains(key)) return m_locals[key];
        return nlohmann::json(nullptr);
    }

    [[nodiscard]] int64_t elapsed_us() const {
        auto now = std::chrono::steady_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(now - m_start_time).count();
    }
};

class Reply {
public:
    using Callback = std::function<void(Result<nlohmann::json>)>;

private:
    struct State {
        std::atomic<bool> completed{false};
        Callback callback;

        explicit State(Callback cb) : completed(false), callback(std::move(cb)) {}

        void complete(Result<nlohmann::json> res) {
            bool expected = false;
            if (completed.compare_exchange_strong(expected, true)) {
                if (callback) {
                    callback(std::move(res));
                }
            }
        }
    };

    std::shared_ptr<State> m_state;

public:
    Reply() = default;

    explicit Reply(Callback callback)
        : m_state(std::shared_ptr<State>(new State(std::move(callback)))) {}

    // Move-only semantics
    Reply(Reply&&) noexcept = default;
    Reply& operator=(Reply&&) noexcept = default;
    Reply(const Reply&) = delete;
    Reply& operator=(const Reply&) = delete;

    ~Reply() {
        if (m_state && !m_state->completed.load()) {
            m_state->complete(make_error(ErrorCode::internal, "Handler completed without sending a reply"));
        }
    }

    void send(nlohmann::json data) {
        if (m_state) {
            m_state->complete(std::move(data));
        }
    }

    void error(ErrorCode code, std::string message, nlohmann::json details = nlohmann::json::object()) {
        if (m_state) {
            m_state->complete(make_error(code, std::move(message), std::move(details)));
        }
    }

    void error(const Error& err) {
        if (m_state) {
            m_state->complete(tl::make_unexpected(err));
        }
    }

    [[nodiscard]] bool is_completed() const noexcept {
        return m_state ? m_state->completed.load() : true;
    }
};

struct CommandSpec {
    std::string name;
    nlohmann::json args_schema;
    std::string required_capability;
    uint32_t timeout_ms{30000};
    size_t max_request_bytes{1048576}; // 1 MiB default

    CommandSpec() = default;

    explicit CommandSpec(std::string cmd_name)
        : name(std::move(cmd_name)), args_schema(nlohmann::json::object()) {}

    CommandSpec& with_capability(std::string cap) {
        required_capability = std::move(cap);
        return *this;
    }

    CommandSpec& with_schema(nlohmann::json schema) {
        args_schema = std::move(schema);
        return *this;
    }

    CommandSpec& with_timeout(uint32_t ms) {
        timeout_ms = ms;
        return *this;
    }

    CommandSpec& with_max_bytes(size_t bytes) {
        max_request_bytes = bytes;
        return *this;
    }
};

using CommandHandler = std::function<void(Context, Reply)>;

} // namespace auraui
