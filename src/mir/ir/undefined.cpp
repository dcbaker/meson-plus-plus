// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "undefined.hpp"
#include "helpers.hpp"

namespace MIR::IR {

Undefined::Undefined() {};

std::string Undefined::serialize(unsigned indent) const {
    return Private::indenter(indent) + "Undefined { }";
}

} // namespace MIR::IR
