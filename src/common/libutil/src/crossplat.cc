#include "libutil/crossplat.h"
#include <filesystem>
#include <array>
#include <climits>
#include <system_error>
#include <unistd.h>
#include <vector>
#include <string>

#ifdef CATTER_LINUX
extern char** environ;
#endif
#if CATTER_MAC
#include <crt_externs.h>
#endif

namespace catter::util {
std::vector<std::string> get_environment() noexcept {
    std::vector<std::string> env_vars;
#if defined(CATTER_MAC)
    auto envp = const_cast<const char**>(*_NSGetEnviron());
    for(int i = 0; envp[i] != nullptr; ++i) {
        env_vars.emplace_back(envp[i]);
    }
#elif defined(CATTER_LINUX)
    for(int i = 0; environ[i] != nullptr; ++i) {
        env_vars.emplace_back(environ[i]);
    }
#elif defined(CATTER_WINDOWS)
// TODO: windows environment variable
#else
#error "Unsupported platform"
#endif
    return env_vars;
}

std::string get_executable_path(std::error_code& ec) {
    std::array<char, PATH_MAX> buf;
#if defined(CATTER_LINUX)
    ssize_t len = readlink("/proc/self/exe", buf.data(), buf.size() - 1);
    if(len <= 0) {
        ec = std::error_code(errno, std::generic_category());
        return {};
    }
    buf[len] = '\0';
#elif defined(CATTER_MAC)
    uint32_t size = buf.size();
    if(_NSGetExecutablePath(buf.data(), &size) != 0) {
        ec = std::error_code(ERANGE, std::generic_category());
        return {};
    }
#endif
    return std::string(buf.data());
}

std::string home_path() {
    const char* home = getenv("HOME");
    if(home == nullptr) {
        throw std::runtime_error("HOME environment variable not set");
    }
    return std::string(home);
}

std::string catter_path() {
    const auto home_p = home_path();
    std::filesystem::path path = home_p;
    path /= ".catter";
    return path.string();
}
}  // namespace catter::util
