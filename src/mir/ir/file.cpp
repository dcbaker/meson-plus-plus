// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "file.hpp"
#include "helpers.hpp"

#include <sstream>

namespace MIR::IR {

using Private::indenter;

File::File(std::string_view s) : path{s} {};

std::string File::serialize(unsigned indent) const {
    std::stringstream ss{};
    // clang-format off
    ss << indenter(indent) << "File { "
       << "path = { " << path << " }"
       << " }";
    // clang-format on
    return ss.str();
}

} // namespace MIR::IR
