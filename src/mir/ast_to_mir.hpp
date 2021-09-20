// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include "mir.hpp"
#include "node.hpp"
#include "state/state.hpp"

namespace MIR {

/// Lower AST to IR
BasicBlock lower_ast(const std::unique_ptr<Frontend::AST::CodeBlock> &,
                     const MIR::State::Persistant &);

}; // namespace MIR
