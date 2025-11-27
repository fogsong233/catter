#include <cstdlib>
#include <filesystem>
#include <string>
#include <system_error>

#include "constructor.h"
#include "librpc/data.h"
#include "librpc/helper.h"
#include "libutil/crossplat.h"
#include "libutil/log.h"
#include "libhook/interface.h"
#include "libconfig/proxy.h"

#include "libutil/output.h"
#include "rpc.h"

namespace catter::proxy {
int run(rpc::data::action act, rpc::data::command_id_t id, std::error_code& ec) {
    using catter::rpc::data::action;
    switch(act.type) {
        case action::WRAP: {
            return std::system(rpc::helper::cmdline_of(act.cmd).c_str());
        }
        case action::INJECT: {
            return catter::proxy::hook::run(act.cmd, id, ec);
        }
        case action::DROP: {
            return 0;
        }
        default: {
            return -1;
        }
    }
}

std::string get_proxy_path() {
    std::error_code ec;
    auto res = catter::util::get_executable_path(ec);
    if(ec) {
        catter::output::redLn("Error getting executable path: {}", ec.message());
        std::abort();
    }
    return res;
};
}  // namespace catter::proxy

// we do not output in proxy, it must be invoked by main program.
int main(int argc, char* argv[], char* envp[]) {
    catter::log::init_logger(
        "catter-proxy.log",
        (std::filesystem::path(catter::util::catter_path()) / catter::config::proxy::LOG_PATH_REL)
            .c_str(),
        false);
    // To let hook in this process stop working
    setenv(catter::config::proxy::CATTER_PROXY_ENV_KEY, "v1", 0);
    // single instance of rpc handler
    auto& rpc_ins = catter::proxy::rpc_handler::instance();

    if(argc < 4) {
        // -p is the parent of this
        LOG_CRITICAL("Expected at least 4 arguments, got {}", argc);
        rpc_ins.report_error(-1, "Insufficient arguments in catter-proxy");
        return -1;
    }

    char** arg_end = argv + argc;

    try {

        if(std::string(argv[1]) != "-p") {
            LOG_CRITICAL("Expected '-p' as the first argument");
            rpc_ins.report_error(-1, "Invalid arguments in catter-proxy");
            return -1;
        }

        catter::rpc::data::command_id_t from_id = std::stoi(argv[2]);

        if(std::string(argv[3]) != "--") {
            if(argv[3] != nullptr) {
                // a msg from hook
                rpc_ins.report_error(from_id, argv[3]);
                return -1;
            }
            LOG_CRITICAL("Expected '--' as the third argument");
            rpc_ins.report_error(-1, "Invalid arguments in catter-proxy");
            return -1;
        }

        // 1. read command from args
        auto cmd = catter::proxy::build_raw_cmd(argv + 4, arg_end);
        std::error_code ec;

        // 2. locate executable, which means resolve PATH if needed
        catter::proxy::hook::locate_exe(cmd, ec);
        if(ec) {
            LOG_ERROR("Failed to locate executable: {}", ec.message());
            rpc_ins.report_error(from_id, "Failed to locate executable: " + ec.message());
            return -1;
        }

        // 3. remote procedure call, wait server make decision
        // TODO, depend yalantinglib, coro_rpc
        // use interface in librpc/function.h
        // now we just invoke the function, it is wrong in use

        auto received_act = rpc_ins.make_decision(from_id, cmd);
        // received cmd maybe not a path, either, so we need locate again
        catter::proxy::hook::locate_exe(received_act.cmd, ec);
        if(ec) {
            LOG_ERROR("Failed to locate executable for received action: {}", ec.message());
            rpc_ins.report_error(from_id,
                                 "Failed to locate executable for received action: " +
                                     ec.message());
            return -1;
        }

        // 4. run command
        int ret = catter::proxy::run(received_act, rpc_ins.new_id(), ec);
        if(ec) {
            LOG_ERROR("Failed to run command: {}", ec.message());
            rpc_ins.report_error(from_id, "Failed to run command: " + ec.message());
            return -1;
        }

        // 5. report finish
        rpc_ins.finish(ret);

        // 5. return exit code
        return ret;
    } catch(const std::exception& e) {
        LOG_CRITICAL("Exception in catter-proxy: {}", e.what());
        rpc_ins.report_error(-1, e.what());
        return -1;
    }
}
