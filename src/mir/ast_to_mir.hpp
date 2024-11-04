// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#pragma once

#include "mir.hpp"
#include "node.hpp"
#include "state/state.hpp"

#include <memory>

namespace MIR {

/// Lower AST to IR
std::shared_ptr<BasicBlock> lower_ast(const std::unique_ptr<Frontend::AST::CodeBlock> &,
                                      const MIR::State::Persistant &);

}; // namespace MIR
