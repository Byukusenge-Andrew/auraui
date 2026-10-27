#pragma once

#include "error.hpp"
#include <tl/expected.hpp>

namespace auraui {

template <typename T>
using Result = tl::expected<T, Error>;

using VoidResult = Result<void>;

inline tl::unexpected<Error> make_error(ErrorCode code, std::string message, nlohmann::json details = nlohmann::json::object()) {
    return tl::make_unexpected(Error(code, std::move(message), std::move(details)));
}

} // namespace auraui
