// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "target.hpp"
#include "helpers.hpp"

namespace MIR::IR {

Target::Target() = default;

std::string Target::serialize(unsigned indent) const {
    return Private::indenter(indent) + "Target { }";
}

} // namespace MIR::IR
