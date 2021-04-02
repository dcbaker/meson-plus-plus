// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/**
 * Logging utilities
 *
 * Codes must reset themselves after each use.
 *
 * These are designed to be used directly with standard C++ printing functions,
 * so a user can just
 * ```c++
 * std::cout << Util::Log::blue("my text") << std::endl
 * ```
 */

#pragma once

#include <string>

namespace Util::Log {

std::string bold(const std::string &);

std::string blue(const std::string &);
std::string green(const std::string &);
std::string red(const std::string &);

}
