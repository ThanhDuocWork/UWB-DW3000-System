#pragma once

#include "error_code.h"

namespace common {

template <typename T>
struct Result {
    T value{};
    ErrorCode error = ErrorCode::Ok;

    [[nodiscard]] bool ok() const
    {
        return error == ErrorCode::Ok;
    }
};

}  // namespace common
