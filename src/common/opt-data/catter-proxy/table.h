#pragma once

#include "option/opt_table.h"

namespace catter::optdata::catter_proxy {
enum OptionID { OPT_INVALID = 0, OPT_INPUT = 1, OPT_UNKNOWN = 2, OPT_PARENT_ID = 2, OPT_EXEC = 3 };

extern opt::OptTable catter_proxy_opt_table;
}  // namespace catter::optdata::catter_proxy
