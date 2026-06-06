#pragma once

#include "app_config.h"

namespace app {

struct Context {
    Config config{};
    bool initialized = false;
};

Context &context();

}  // namespace app
