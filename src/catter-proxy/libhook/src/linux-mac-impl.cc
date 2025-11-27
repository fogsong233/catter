#include "libhook/interface.h"
#include "libconfig/linux-mac-hook.h"
#include "libconfig/proxy.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <format>
#include <limits.h>
#include "libconfig/linux-mac-hook.h"
#include <spawn.h>
#include <system_error>
#include <unistd.h>
#include <dirent.h>
#include <sys/wait.h>
#include "librpc/data.h"
#include "libutil/crossplat.h"
#include "libutil/log.h"

/**
 * @brief Run a command using posix_spawn
 * @param command The command to run
 * @return The exit code of the command
 */
static int run_command(const catter::rpc::data::command& command) {
    std::vector<char*> argv_ptrs;
    // add argv[0]
    argv_ptrs.push_back(const_cast<char*>(command.executable.c_str()));
    for(auto& arg: command.args) {
        argv_ptrs.push_back(const_cast<char*>(arg.c_str()));
    }
    argv_ptrs.push_back(nullptr);

    std::vector<char*> envp_ptrs;
    for(auto& env: command.env) {
        envp_ptrs.push_back(const_cast<char*>(env.c_str()));
    }
    envp_ptrs.push_back(nullptr);
    pid_t pid = 0;
    int spawn_res = posix_spawn(&pid,
                                command.executable.c_str(),
                                nullptr,  // file actions use parent
                                nullptr,  // spawn attributes use parent
                                argv_ptrs.data(),
                                envp_ptrs.data());
    if(spawn_res != 0) {
        return -1;
    }

    int status = 0;
    if(waitpid(pid, &status, 0) == -1) {
        return -1;
    }

    if(WIFEXITED(status)) {
        return WEXITSTATUS(status);
    }
    return 0;
}

namespace catter::proxy::hook {

void locate_exe(rpc::data::command& command, std::error_code& ec) {
    std::string result;
    std::array<char, 128> buffer;

    // we use `command -v` instead of `which`, because formmer is in POSIX standard.
    std::string find_cmd = "command -v " + command.executable;
    auto fp = popen(find_cmd.c_str(), "r");
    if(!fp) {
        ec = std::make_error_code(std::errc::io_error);
        return;
    }
    if(fgets(buffer.data(), buffer.size(), fp) != nullptr) {
        result = buffer.data();
        // remove trailing newline
        if(!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
    }
    auto ret = pclose(fp);
    if(ret == -1 || WEXITSTATUS(ret) != 0) {
        ec = std::make_error_code(std::errc::no_such_file_or_directory);
        return;
    }
    if(result.empty()) {
        ec = std::make_error_code(std::errc::no_such_file_or_directory);
        return;
    }
    command.executable = result;
}

std::filesystem::path get_hook_path(std::error_code& ec) {
    auto exe_path = util::get_executable_path(ec);
    if(ec) {
        return {};
    }
    return std::filesystem::path(exe_path).parent_path() /
           catter::config::hook::RELATIVE_PATH_OF_HOOK_LIB;
}

int run(rpc::data::command command, rpc::data::command_id_t id, std::error_code& ec) {
    const auto lib_path = get_hook_path(ec);
    if(ec) {
        return -1;
    }
    LOG_INFO("new command id is: {}", id);
    // check hook_lib exists
    if(!std::filesystem::exists(lib_path, ec)) {
        ec = std::make_error_code(std::errc::no_such_file_or_directory);
        return -1;
    }
    std::string joined_command = "";
#ifdef CATTER_MAC
#ifdef DEBUG
    joined_command += "DYLD_PRINT_INTERPOSING=1 ";
#endif  // DEBUG
#endif  // CATTER_MAC

    command.env.push_back(std::format("{}={}",
                                      //   "/usr/lib/gcc/x86_64-linux-gnu/14/libasan.so",
                                      catter::config::hook::KEY_PRELOAD,
                                      lib_path.string()));
    command.env.push_back(std::format("{}={}", catter::config::hook::KEY_CATTER_COMMAND_ID, id));
    command.env.push_back(
        std::format("{}={}", catter::config::hook::KEY_CATTER_PROXY_PATH, get_proxy_path()));
    // remove CATTER_PROXY_ENV_KEY from env to enable hooking in the child process
    auto rm_it =
        std::remove_if(command.env.begin(), command.env.end(), [](const std::string& env_entry) {
            return env_entry.starts_with(config::proxy::CATTER_PROXY_ENV_KEY);
        });
    command.env.erase(rm_it, command.env.end());

    std::string cmd_for_print = "";
    cmd_for_print += command.executable;
    for(auto& arg: command.args) {
        cmd_for_print += " " + arg;
    }
    LOG_INFO("| -> Catter-Proxy Final Executing command: {}", cmd_for_print);
    return run_command(command);
};

};  // namespace catter::proxy::hook
