// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "operation.hpp"

#include <sstream>

namespace MIR {

Operation::Operation() = default;

std::string Operation::serialize() const { return "Operation { }"; }

} // namespace MIR
