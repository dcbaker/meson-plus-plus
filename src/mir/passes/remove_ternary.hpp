// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

# pragma once

#include "ir/graph.hpp"

namespace MIR::Passes {

bool remove_ternary(std::shared_ptr<IR::Node> node);

}
