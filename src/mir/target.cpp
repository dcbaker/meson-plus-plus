// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "target.hpp"

namespace MIR {

Target::Target() = default;

std::string Target::serialize() const { return "Target { }"; }

} // namespace MIR
