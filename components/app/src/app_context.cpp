#include "app_context.h"

namespace app {

Context &context()
{
    static Context ctx{};
    return ctx;
}

}  // namespace app
