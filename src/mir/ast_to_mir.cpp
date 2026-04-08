// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "ast_to_mir.hpp"

namespace MIR {

namespace {

using namespace Frontend;

/// @brief Lower AST expressions into MIR representations
struct ExpressionLowering {
    IR::InstructionType operator()(const std::unique_ptr<AST::AdditiveExpression> & stmt) const;
    IR::InstructionType operator()(const std::unique_ptr<AST::Boolean> & stmt) const;
    IR::InstructionType operator()(const std::unique_ptr<AST::Identifier> & stmt) const;
    IR::InstructionType
    operator()(const std::unique_ptr<AST::MultiplicativeExpression> & stmt) const;
    IR::InstructionType operator()(const std::unique_ptr<AST::UnaryExpression> & stmt) const;
    IR::InstructionType operator()(const std::unique_ptr<AST::Number> & stmt) const;
    IR::InstructionType operator()(const std::unique_ptr<AST::String> & stmt) const;
    IR::InstructionType operator()(const std::unique_ptr<AST::Subscript> & stmt) const;
    IR::InstructionType operator()(const std::unique_ptr<AST::Relational> & stmt) const;
    IR::InstructionType operator()(const std::unique_ptr<AST::FunctionCall> & stmt) const;
    IR::InstructionType operator()(const std::unique_ptr<AST::GetAttribute> & stmt) const;
    IR::InstructionType operator()(const std::unique_ptr<AST::Array> & stmt) const;
    IR::InstructionType operator()(const std::unique_ptr<AST::Dict> & stmt) const;
    IR::InstructionType operator()(const std::unique_ptr<AST::Ternary> & stmt) const;
};

/// @brief Lower AST statements into MIR representations
struct StatementLowering {

    StatementLowering() : el{} {};

    IR::Instruction operator()(const std::unique_ptr<AST::Statement> & stmt) const;
    IR::Instruction operator()(const std::unique_ptr<AST::Assignment> & stmt) const;
    IR::Instruction operator()(const std::unique_ptr<AST::IfStatement> & stmt) const;
    IR::Instruction operator()(const std::unique_ptr<AST::ForeachStatement> & stmt) const;
    IR::Instruction operator()(const std::unique_ptr<AST::Break> & stmt) const;
    IR::Instruction operator()(const std::unique_ptr<AST::Continue> & stmt) const;

  private:
    const ExpressionLowering el;
};

} // namespace

IR::CFG ast_to_mir(const std::unique_ptr<Frontend::AST::CodeBlock> & block) {
    return IR::CFG{std::make_shared<IR::Node>(std::make_shared<IR::BasicBlock>())};
}

} // namespace MIR
