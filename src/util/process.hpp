// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/**
 * Interface for calling external processes
 */

#pragma once

#include <string>
#include <tuple>

namespace Util {

std::tuple<int, std::string, std::string> process();

}; // namespace Util
