// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "string.hpp"

namespace MIR::IR {

String::String() = default;

std::string String::serialize() const { return "String { }"; }

}
