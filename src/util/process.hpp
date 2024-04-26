// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

/**
 * Interface for calling external processes
 */

#pragma once

#include <cstdint>
#include <string>
#include <tuple>
#include <vector>

namespace Util {

/**
 * The result of a process.
 *
 * In the form (returncode, stdout, stderr)
 */
using Result = std::tuple<int8_t, std::string, std::string>;

/**
 * Run an external process in a thread, and return the output, stdout, and stderr
 *
 * Can be configured to not return stdout and stderr, in which case it will
 * just surpress them
 */
Result process(const std::vector<std::string> &);

}; // namespace Util
