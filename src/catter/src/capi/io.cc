#include <print>
#include "../apitool.h"
#include "libqjs/qjs.h"

namespace {
CAPI(stdout_print, (const std::string content)->void) {
    std::print("{}", content);
}
}  // namespace
