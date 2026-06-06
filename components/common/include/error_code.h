#pragma once

namespace common {

enum class ErrorCode {
    Ok = 0,
    InvalidArgument,
    NotInitialized,
    NotSupported,
    Timeout,
    Io,
    Internal,
};

const char *to_string(ErrorCode code);

}  // namespace common
