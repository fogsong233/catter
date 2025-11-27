#pragma once

#include "librpc/data.h"
#include <system_error>

namespace catter::proxy::hook {
/// the hook impl should also locate executable if needed
void locate_exe(rpc::data::command& command, std::error_code& ec);

/// Run the command with catter proxy hook
int run(rpc::data::command command, rpc::data::command_id_t id, std::error_code& ec);
}  // namespace catter::proxy::hook

/// The catter proxy should provide its own path
namespace catter::proxy {
std::string get_proxy_path();
}  // namespace catter::proxy
