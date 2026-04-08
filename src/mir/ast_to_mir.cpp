// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "ast_to_mir.hpp"

namespace MIR {

namespace {

using namespace Frontend;

/// @brief Lower AST expressions into MIR representations
struct ExpressionLowering {
    IR::InstructionType operator()(const std::unique_ptr<AST::AdditiveExpression> & stmt) const {
        IR::Operation2SrcType op;
        switch (stmt->op) {
            case AST::AddOp::ADD:
                op = IR::Operation2SrcType::add;
                break;
            case AST::AddOp::SUB:
                op = IR::Operation2SrcType::sub;
                break;
            default:
                throw std::runtime_error{"Unknown additive expression type"};
        }

        IR::InstructionType && lhs = std::visit(*this, stmt->lhs);
        IR::InstructionType && rhs = std::visit(*this, stmt->rhs);
        return std::make_shared<IR::Operation2Src>(lhs, op, rhs);
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Boolean> & stmt) const {
        return std::make_shared<IR::Boolean>(stmt->value);
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Identifier> & stmt) const {
        return std::make_shared<IR::Identifier>(stmt->value);
    }

    IR::InstructionType
    operator()(const std::unique_ptr<AST::MultiplicativeExpression> & stmt) const {
        IR::Operation2SrcType op;
        switch (stmt->op) {
            case AST::MulOp::MOD:
                op = IR::Operation2SrcType::mod;
                break;
            case AST::MulOp::MUL:
                op = IR::Operation2SrcType::mul;
                break;
            case AST::MulOp::DIV:
                op = IR::Operation2SrcType::div;
                break;
            default:
                throw std::runtime_error{"Unknown multiplication expression type"};
        }

        IR::InstructionType && lhs = std::visit(*this, stmt->lhs);
        IR::InstructionType && rhs = std::visit(*this, stmt->rhs);
        return std::make_shared<IR::Operation2Src>(lhs, op, rhs);
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::UnaryExpression> & stmt) const {
        IR::Operation1SrcType op;
        switch (stmt->op) {
            case AST::UnaryOp::NEG:
                op = IR::Operation1SrcType::negate;
                break;
            case AST::UnaryOp::NOT:
                op = IR::Operation1SrcType::lnot;
                break;
            default:
                throw std::runtime_error{"Unknown unary expression type"};
        }

        IR::InstructionType && value = std::visit(*this, stmt->rhs);
        return std::make_shared<IR::Operation1Src>(value, op);
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Number> & stmt) const {
        return std::make_shared<IR::Number>(stmt->value);
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::String> & stmt) const {
        return std::make_shared<IR::String>(stmt->value);
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Subscript> & stmt) const {
        IR::InstructionType && container = std::visit(*this, stmt->lhs);
        IR::InstructionType && index = std::visit(*this, stmt->rhs);
        return std::make_shared<IR::Operation2Src>(container, IR::Operation2SrcType::subscript,
                                                   index);
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Relational> & stmt) const {
        IR::Operation2SrcType op;
        switch (stmt->op) {
            case AST::RelationalOp::AND:
                op = IR::Operation2SrcType::and_;
                break;
            case AST::RelationalOp::OR:
                op = IR::Operation2SrcType::or_;
                break;
            case AST::RelationalOp::EQ:
                op = IR::Operation2SrcType::eq;
                break;
            case AST::RelationalOp::NE:
                op = IR::Operation2SrcType::ne;
                break;
            case AST::RelationalOp::GE:
                op = IR::Operation2SrcType::ge;
                break;
            case AST::RelationalOp::GT:
                op = IR::Operation2SrcType::gt;
                break;
            case AST::RelationalOp::LT:
                op = IR::Operation2SrcType::lt;
                break;
            case AST::RelationalOp::LE:
                op = IR::Operation2SrcType::le;
                break;
            case AST::RelationalOp::NOT_IN:
                op = IR::Operation2SrcType::not_in;
                break;
            case AST::RelationalOp::IN:
                op = IR::Operation2SrcType::in;
                break;
            default:
                throw std::runtime_error{"Unknown relation expression type"};
        }

        IR::InstructionType && lhs = std::visit(*this, stmt->lhs);
        IR::InstructionType && rhs = std::visit(*this, stmt->rhs);
        return std::make_shared<IR::Operation2Src>(lhs, op, rhs);
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::FunctionCall> & stmt) const;

    IR::InstructionType operator()(const std::unique_ptr<AST::GetAttribute> & stmt) const {
        IR::InstructionType && holder = std::visit(*this, stmt->holder);
        IR::InstructionType && held = std::visit(*this, stmt->held);
        return std::make_shared<IR::Operation2Src>(holder, IR::Operation2SrcType::get_attribute,
                                                   held);
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Array> & stmt) const {
        std::vector<IR::InstructionType> held;
        for (auto && v : stmt->elements) {
            held.emplace_back(std::visit(*this, v));
        }
        return std::make_shared<IR::Array>(std::move(held));
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Dict> & stmt) const {
        std::map<IR::InstructionType, IR::InstructionType> value;
        for (auto && [k, v] : stmt->elements) {
            value.emplace(std::visit(*this, k), std::visit(*this, v));
        }

        std::make_shared<IR::Dict>(std::move(value));
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Ternary> & stmt) const {
        IR::InstructionType && cond = std::visit(*this, stmt->condition);
        IR::InstructionType && lhs = std::visit(*this, stmt->lhs);
        IR::InstructionType && rhs = std::visit(*this, stmt->rhs);
        return std::make_shared<IR::Ternary>(cond, lhs, rhs);
    }
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
