// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include "mir.hpp"
#include "node.hpp"

namespace MIR {

/// Lower AST to IR
IRList lower_ast(const std::unique_ptr<Frontend::AST::CodeBlock> &);

}; // namespace MIR
