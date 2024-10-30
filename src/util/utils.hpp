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

/// @brief Join a vector of strings with a given delimiter
/// @param strs the vector to join
/// @param delim the delimiter to use
/// @return a string
std::string join(const std::vector<std::string> & strs, std::string_view delim = "");

/// @brief Replace a substring with another one, returning a new string
/// @param src  The string to be modified
/// @param replace  The target value to be replaced
/// @param with  The value to replace with
/// @return A new string with all instances of `replace` replaced by `with`
std::string replace(std::string src, std::string_view replace, std::string_view with);

/// @brief Quote a string to be Makefile compatible
/// @param src The string to be quoted
/// @return a new string that is properly quoted
std::string makefile_quote(std::string src);

} // namespace Util
