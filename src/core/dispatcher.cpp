#include "dispatcher.hpp"
#include "validator.hpp"
#include "auraui/log.hpp"

#include <iostream>
#include <stdexcept>

namespace auraui {
namespace core {

Dispatcher::Dispatcher(security::CapabilityPolicy policy, uint32_t rate_limit_per_sec, std::shared_ptr<ThreadPool> pool)
    : m_policy(std::move(policy)),
      m_rate_limiter(rate_limit_per_sec),
      m_pool(std::move(pool)) {
    if (!m_pool) {
        m_pool = std::make_shared<ThreadPool>(4);
    }
}

Result<void> Dispatcher::register_command(CommandSpec spec, CommandHandler handler, std::vector<Middleware> mws) {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (spec.name.empty()) {
        return make_error(ErrorCode::invalid_argument, "Cannot register empty command name");
    }

    if (m_commands.find(spec.name) != m_commands.end()) {
        return make_error(ErrorCode::invalid_argument, "Command collision: '" + spec.name + "' is already registered");
    }

    std::string name = spec.name;
    m_commands.insert_or_assign(name, RegisteredCommand{
        std::move(spec),
        std::move(handler),
        std::move(mws)
    });
    return {};
}

void Dispatcher::use(Middleware mw) {
    std::lock_guard<std::mutex> lock(m_mutex);
    m_global_middlewares.push_back(std::move(mw));
}

void Dispatcher::mount(const std::string& prefix, const Router& router) {
    for (const auto& entry : router.routes()) {
        std::string full_name = prefix.empty() ? entry.path : (prefix + "." + entry.path);
        CommandSpec spec = entry.spec;
        spec.name = full_name;

        std::vector<Middleware> combined_mws = router.middlewares();
        combined_mws.insert(combined_mws.end(), entry.middlewares.begin(), entry.middlewares.end());

        auto res = register_command(std::move(spec), entry.handler, std::move(combined_mws));
        if (!res) {
            throw std::runtime_error("AuraUI Route Collision during mount: " + res.error().message());
        }
    }

    for (const auto& sub : router.subrouters()) {
        std::string next_prefix = prefix.empty() ? sub.first : (prefix + "." + sub.first);
        mount(next_prefix, sub.second);
    }
}

void Dispatcher::dispatch(std::string_view raw_request_json, std::function<void(std::string)> reply_callback) {
    // Step 1: Parse request envelope
    auto req_res = IpcEnvelope::parse_request(raw_request_json);
    if (!req_res) {
        Logger::warn("Dispatcher", "Envelope parse rejected: " + req_res.error().message());
        reply_callback(IpcEnvelope::serialize_response(IpcResponse::fail("", req_res.error())));
        return;
    }

    IpcRequest req = std::move(*req_res);
    std::string req_id = req.id;
    std::string cmd_name = req.cmd;

    // Step 2: Rate limit check
    auto rate_res = m_rate_limiter.check(cmd_name);
    if (!rate_res) {
        Logger::warn("Dispatcher", "Rate limit exceeded for command '" + cmd_name + "'");
        reply_callback(IpcEnvelope::serialize_response(IpcResponse::fail(req_id, rate_res.error())));
        return;
    }

    RegisteredCommand cmd_entry;
    std::vector<Middleware> all_mws;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        auto it = m_commands.find(cmd_name);
        if (it == m_commands.end()) {
            Logger::warn("Dispatcher", "Command not found: '" + cmd_name + "'");
            reply_callback(IpcEnvelope::serialize_response(IpcResponse::fail(
                req_id, make_error(ErrorCode::not_found, "Command '" + cmd_name + "' not registered in backend"))));
            return;
        }
        cmd_entry = it->second;
        all_mws = m_global_middlewares;
        all_mws.insert(all_mws.end(), cmd_entry.middlewares.begin(), cmd_entry.middlewares.end());
    }

    // Step 3: Capability check (Deny-by-default)
    if (!cmd_entry.spec.required_capability.empty()) {
        if (!m_policy.has_capability(cmd_entry.spec.required_capability)) {
            std::string reason = "Capability '" + cmd_entry.spec.required_capability + "' is denied by application manifest";
            Logger::warn("Security", "DENIAL: cmd='" + cmd_name + "', reason='" + reason + "'");
            reply_callback(IpcEnvelope::serialize_response(IpcResponse::fail(
                req_id, make_error(ErrorCode::permission_denied, reason))));
            return;
        }
    }

    // Step 4: Schema validation
    auto val_res = Validator::validate_args(req.args, cmd_entry.spec.args_schema);
    if (!val_res) {
        Logger::warn("Dispatcher", "Schema validation failed for '" + cmd_name + "': " + val_res.error().message());
        reply_callback(IpcEnvelope::serialize_response(IpcResponse::fail(req_id, val_res.error())));
        return;
    }

    // Step 5: Execute pipeline
    Context ctx(cmd_name, req_id, std::move(req.args));

    Reply reply([req_id, reply_callback](Result<nlohmann::json> result) {
        if (result) {
            reply_callback(IpcEnvelope::serialize_response(IpcResponse::success(req_id, std::move(*result))));
        } else {
            reply_callback(IpcEnvelope::serialize_response(IpcResponse::fail(req_id, result.error())));
        }
    });

    // Execute in worker pool so handlers don't block the UI thread
    struct Runner : std::enable_shared_from_this<Runner> {
        Context ctx;
        Reply reply;
        std::vector<Middleware> mws;
        CommandHandler handler;
        size_t idx{0};

        Runner(Context c, Reply r, std::vector<Middleware> m, CommandHandler h)
            : ctx(std::move(c)), reply(std::move(r)), mws(std::move(m)), handler(std::move(h)) {}

        void run_next() {
            if (reply.is_completed()) return;

            if (idx < mws.size()) {
                size_t cur = idx++;
                auto self = shared_from_this();
                try {
                    mws[cur](ctx, Next([self](std::optional<Error> err) {
                        if (err.has_value()) {
                            self->reply.error(err.value());
                        } else {
                            self->run_next();
                        }
                    }));
                } catch (const std::exception& ex) {
                    reply.error(ErrorCode::internal, std::string("Middleware exception: ") + ex.what());
                } catch (...) {
                    reply.error(ErrorCode::internal, "Unknown middleware exception");
                }
            } else {
                try {
                    handler(std::move(ctx), std::move(reply));
                } catch (const std::exception& ex) {
                    reply.error(ErrorCode::internal, std::string("Handler exception: ") + ex.what());
                } catch (...) {
                    reply.error(ErrorCode::internal, "Unknown handler exception");
                }
            }
        }
    };

    auto runner = std::make_shared<Runner>(std::move(ctx), std::move(reply), std::move(all_mws), std::move(cmd_entry.handler));
    m_pool->post([runner]() {
        runner->run_next();
    });
}

} // namespace core
} // namespace auraui
