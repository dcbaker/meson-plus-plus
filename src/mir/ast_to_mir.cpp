// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "ast_to_mir.hpp"

namespace MIR {

namespace {

/// @brief Lower AST expressions into MIR representations
struct ExpressionLowering {};

/// @brief Lower AST statements into MIR representations
struct StatementLowering {};

} // namespace

IR::CFG ast_to_mir(const std::unique_ptr<Frontend::AST::CodeBlock> & block) {
    return IR::CFG{std::make_shared<IR::Node>(std::make_shared<IR::BasicBlock>())};
}

} // namespace MIR
