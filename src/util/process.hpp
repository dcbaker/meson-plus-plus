// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/**
 * Interface for calling external processes
 */

#pragma once

#include <string>
#include <tuple>
#include <vector>
#include <cstdint>

namespace Util {

/**
 * The result of a process.
 *
 * In the form (returncode, stdout, stderr)
 */
typedef std::tuple<int8_t, std::string, std::string> Result;

/**
 * Run an external process in a thread, and return the output, stdout, and stderr
 *
 * Can be configured to not return stdout and stderr, in which case it will
 * just surpress them
 */
Result process(const std::vector<std::string> &);

}; // namespace Util
