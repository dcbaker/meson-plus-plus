// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#pragma once

#include "ir.hpp"
#include "node.hpp"

namespace MIR {

IR::CFG ast_to_mir(const std::unique_ptr<Frontend::AST::CodeBlock> & block);

}
