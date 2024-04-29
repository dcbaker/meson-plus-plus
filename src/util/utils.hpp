// SPDX-License-Indentifier: Apache-2.0
// Copyright © 2024 Intel Corporation

/// Catch all for generic utilities

#include <string>
#include <vector>

namespace Util {

/// @brief Split a string ona given delimiter
/// @param input The string to be split
/// @param delim The value to split on, defaults to '\n'
/// @return A vector of strings
std::vector<std::string> split(std::string_view input, std::string_view delim = "\n");

} // namespace Util
