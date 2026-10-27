#pragma once

#include "auraui/result.hpp"
#include "auraui/error.hpp"
#include "nlohmann/json.hpp"
#include <string>
#include <string_view>

namespace auraui {
namespace core {

struct IpcRequest {
    int version{1};
    std::string id;
    std::string cmd;
    nlohmann::json args;
};

struct IpcResponse {
    int version{1};
    std::string id;
    bool ok{true};
    nlohmann::json data{nullptr};
    std::optional<Error> error;

    static IpcResponse success(std::string id, nlohmann::json data) {
        return IpcResponse{1, std::move(id), true, std::move(data), std::nullopt};
    }

    static IpcResponse fail(std::string id, Error err) {
        return IpcResponse{1, std::move(id), false, nullptr, std::move(err)};
    }

    static IpcResponse fail(std::string id, const tl::unexpected<Error>& err) {
        return IpcResponse{1, std::move(id), false, nullptr, err.value()};
    }
};

class IpcEnvelope {
public:
    static constexpr size_t MAX_PAYLOAD_BYTES = 1048576; // 1 MiB limit
    static constexpr size_t MAX_ID_LENGTH = 64;
    static constexpr int CURRENT_VERSION = 1;

    [[nodiscard]] static Result<IpcRequest> parse_request(std::string_view json_str);
    [[nodiscard]] static std::string serialize_response(const IpcResponse& response) noexcept;
};

} // namespace core
} // namespace auraui
