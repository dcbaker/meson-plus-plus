// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include "node.hpp"
#include "ir.hpp"

namespace IR {

/// Lower AST to IR
BlockList lower_ast(const Frontend::AST::CodeBlock &);

};
