// SPDX-License-Indentifier: Apache-2.0
// Copyright © 2024 Intel Corporation

#include "utils.hpp"

#include <sstream>

namespace Util {

std::vector<std::string> split(std::string_view input, std::string_view delim) {
    size_t last = 0, next = 0;
    std::vector<std::string> out;

    while ((next = input.find(delim, last)) != std::string_view::npos) {
        out.emplace_back(input.substr(last, next - last));
        last = next + 1;
    }
    out.emplace_back(input.substr(last));

    return out;
}

std::string join(const std::vector<std::string> & strs, std::string_view delim) {
    if (strs.empty()) {
        return "";
    }
    std::stringstream ss{};

    auto itr = strs.begin();
    for (; itr < (strs.end() - 1); ++itr) {
        ss << *itr << delim;
    }
    // Insert the last element without the delimiter
    ss << *itr;

    return ss.str();
}

} // namespace Util
