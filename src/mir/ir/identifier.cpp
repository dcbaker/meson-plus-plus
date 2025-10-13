// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "identifier.hpp"

namespace MIR::IR {

Identifier::Identifier() = default;

std::string Identifier::serialize() const { return "Identifier { }"; }

} // namespace MIR::IR
