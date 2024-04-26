// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include "log.hpp"

namespace Util::Log {

namespace {

const std::string RESET = "\033[0m";

}

std::string blue(const std::string & s) { return "\033[34m" + s + RESET; };

std::string green(const std::string & s) { return "\033[32m" + s + RESET; };

std::string red(const std::string & s) { return "\033[31m" + s + RESET; };

std::string yellow(const std::string & s) { return "\033[33m" + s + RESET; };

std::string bold(const std::string & s) { return "\033[1m" + s + RESET; };

} // namespace Util::Log
