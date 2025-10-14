// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "file.hpp"

#include <sstream>

namespace MIR::IR {

File::File(std::string_view s) : path{s} {};

std::string File::serialize() const {
    std::stringstream ss{};
    ss << "File { "
       << "path = { " << path << " } "
       << "}";
    return ss.str();
}

} // namespace MIR::IR
