#include "ipc_envelope.hpp"
#include "utf8.hpp"

namespace auraui {
namespace core {

Result<IpcRequest> IpcEnvelope::parse_request(std::string_view json_str) {
    if (json_str.empty()) {
        return make_error(ErrorCode::invalid_argument, "Empty IPC payload");
    }

    if (json_str.size() > MAX_PAYLOAD_BYTES) {
        return make_error(ErrorCode::invalid_argument, "IPC payload exceeds maximum limit of 1 MiB");
    }

    if (!Utf8::is_valid(json_str)) {
        return make_error(ErrorCode::invalid_argument, "IPC payload contains invalid UTF-8 bytes");
    }

    nlohmann::json j;
    try {
        j = nlohmann::json::parse(json_str);
    } catch (const std::exception& ex) {
        return make_error(ErrorCode::invalid_argument, std::string("Malformed JSON: ") + ex.what());
    }

    if (!j.is_object()) {
        return make_error(ErrorCode::invalid_argument, "IPC request envelope must be a JSON object");
    }

    // Check version
    if (!j.contains("v") || !j["v"].is_number_integer()) {
        return make_error(ErrorCode::invalid_argument, "Missing or invalid protocol version 'v'");
    }

    int version = j["v"].get<int>();
    if (version != CURRENT_VERSION) {
        return make_error(ErrorCode::unsupported, "Unsupported protocol version: " + std::to_string(version));
    }

    // Check id
    if (!j.contains("id") || !j["id"].is_string()) {
        return make_error(ErrorCode::invalid_argument, "Missing or invalid request 'id'");
    }

    std::string id = j["id"].get<std::string>();
    if (id.empty() || id.size() > MAX_ID_LENGTH) {
        return make_error(ErrorCode::invalid_argument, "Request id must be between 1 and 64 characters");
    }

    // Check cmd
    if (!j.contains("cmd") || !j["cmd"].is_string()) {
        return make_error(ErrorCode::invalid_argument, "Missing or invalid command 'cmd'");
    }

    std::string cmd = j["cmd"].get<std::string>();
    if (cmd.empty()) {
        return make_error(ErrorCode::invalid_argument, "Command string cannot be empty");
    }

    // Args (default to empty object if not provided)
    nlohmann::json args = nlohmann::json::object();
    if (j.contains("args")) {
        args = j["args"];
    }

    return IpcRequest{version, std::move(id), std::move(cmd), std::move(args)};
}

std::string IpcEnvelope::serialize_response(const IpcResponse& response) noexcept {
    try {
        nlohmann::json out{
            {"v", response.version},
            {"id", response.id},
            {"ok", response.ok}
        };

        if (response.ok) {
            out["data"] = response.data.is_null() ? nlohmann::json::object() : response.data;
        } else if (response.error.has_value()) {
            out["error"] = response.error.value().to_json();
        } else {
            out["error"] = nlohmann::json{
                {"code", "internal"},
                {"message", "Unknown internal error"}
            };
        }

        return out.dump(-1, ' ', false, nlohmann::json::error_handler_t::replace);
    } catch (...) {
        return "{\"v\":1,\"id\":\"\",\"ok\":false,\"error\":{\"code\":\"internal\",\"message\":\"Serialization failure\"}}";
    }
}

} // namespace core
} // namespace auraui
