#pragma once
#include <system_error>
#include <vector>
#include <string>
#include "librpc/data.h"

namespace catter::util {
std::vector<std::string> get_environment() noexcept;

std::string get_executable_path(std::error_code& ec);

/**
 * @return the home path of current user
 * @throw std::runtime_error if HOME environment variable is not set
 */
std::string home_path();

/**
 * @return the catter path under home directory
 * @throw std::runtime_error if HOME environment variable is not set
 * @example /home/user/.catter
 */
std::string catter_path();
}  // namespace catter::util
