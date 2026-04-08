// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "undefined.hpp"

namespace MIR::IR {

Undefined::Undefined() {};

std::string Undefined::serialize() const { return "Undefined { }"; }

} // namespace MIR::IR
