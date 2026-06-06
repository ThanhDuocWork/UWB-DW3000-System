#include "error_code.h"

namespace common {

const char *to_string(ErrorCode code)
{
    switch (code) {
    case ErrorCode::Ok:
        return "ok";
    case ErrorCode::InvalidArgument:
        return "invalid_argument";
    case ErrorCode::NotInitialized:
        return "not_initialized";
    case ErrorCode::NotSupported:
        return "not_supported";
    case ErrorCode::Timeout:
        return "timeout";
    case ErrorCode::Io:
        return "io";
    case ErrorCode::Internal:
    default:
        return "internal";
    }
}

}  // namespace common
