// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "ast_to_mir.hpp"
#include "ir/instruction.hpp"

#include <memory>
#include <stdexcept>

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

    IR::InstructionType operator()(const std::unique_ptr<AST::FunctionCall> & stmt) const {
        IR::PositionalArguments && pos{};
        for (auto && a : stmt->args->positional) {
            pos.emplace_back(std::visit(*this, a));
        }

        IR::KeywordArguments && kws{};
        for (auto && [k, v] : stmt->args->keyword) {
            pos.emplace_back((std::visit(*this, k), std::visit(*this, v)));
        }

        IR::InstructionType && fname = std::visit(*this, stmt->held);
        // TODO: error handling
        std::string name = std::get<std::shared_ptr<IR::String>>(fname)->m_value;

        return std::make_shared<IR::FunctionCall>(name, std::move(pos), std::move(kws));
    }

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

        return std::make_shared<IR::Dict>(std::move(value));
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Ternary> & stmt) const {
        IR::InstructionType && cond = std::visit(*this, stmt->condition);
        IR::InstructionType && lhs = std::visit(*this, stmt->lhs);
        IR::InstructionType && rhs = std::visit(*this, stmt->rhs);
        return std::make_shared<IR::Ternary>(cond, lhs, rhs);
    }
};

struct StatementLowering;

/// @brief State passed between StatementLowering calls
struct LoweringState {
    std::shared_ptr<IR::Node> current_node;
    std::shared_ptr<IR::Node> loop_header;
    std::shared_ptr<IR::Node> loop_tail;
};

std::shared_ptr<IR::Node> lower_block(const AST::CodeBlock & block, const StatementLowering & lower,
                                      LoweringState & state);

/// @brief Lower AST statements into MIR representations
struct StatementLowering {

    StatementLowering() : el{} {};

    StatementLowering(StatementLowering &&) = delete;
    StatementLowering & operator=(StatementLowering &&) = delete;

    void operator()(const std::unique_ptr<AST::Statement> & stmt, LoweringState & state) const {
        state.current_node->block->instructions.emplace_back(
            std::make_unique<IR::Instruction>(std::visit(el, stmt->expr)));
    }

    void operator()(const std::unique_ptr<AST::Assignment> & stmt, LoweringState & state) const {
        IR::InstructionType lhs = std::visit(el, stmt->lhs);
        // TODO: error handling
        auto & id = std::get<std::shared_ptr<IR::Identifier>>(lhs);
        IR::InstructionType && rhs = std::visit(el, stmt->rhs);

        // In Meson operators like x *= y are short for x = x * y
        // As such, MIR doesn't have representations for them, and they're easy to convert
        // At the AST -> MIR barrier
        switch (stmt->op) {
            case AST::AssignOp::EQUAL:
                break;
            case AST::AssignOp::ADD_EQUAL:
                rhs = std::make_shared<IR::Operation2Src>(
                    std::move(lhs), IR::Operation2SrcType::add, std::move(rhs));
                break;
            case AST::AssignOp::SUB_EQUAL:
                rhs = std::make_shared<IR::Operation2Src>(
                    std::move(lhs), IR::Operation2SrcType::sub, std::move(rhs));
                break;
            case AST::AssignOp::DIV_EQUAL:
                rhs = std::make_shared<IR::Operation2Src>(
                    std::move(lhs), IR::Operation2SrcType::div, std::move(rhs));
                break;
            case AST::AssignOp::MUL_EQUAL:
                rhs = std::make_shared<IR::Operation2Src>(
                    std::move(lhs), IR::Operation2SrcType::mul, std::move(rhs));
                break;
            case AST::AssignOp::MOD_EQUAL:
                rhs = std::make_shared<IR::Operation2Src>(
                    std::move(lhs), IR::Operation2SrcType::mod, std::move(rhs));
                break;
            default:
                throw std::runtime_error{"Unknown operator"};
        }

        state.current_node->block->instructions.emplace_back(
            std::make_unique<IR::Instruction>(std::move(rhs), IR::Variable{id->m_name}));
    }

    void operator()(const std::unique_ptr<AST::IfStatement> & stmt, LoweringState & state) const {
        // This is the block that all of the branches of the if/elif/else web
        // will join back to
        auto tail = std::make_shared<IR::Node>();

        // place the condition as the last instruction of the block.
        // TODO: We might need a Condition{} type?
        state.current_node->block->instructions.emplace_back(
            std::make_unique<IR::Instruction>(std::visit(el, stmt->ifblock.condition)));

        std::shared_ptr<IR::Node> lhs = nullptr;

        // create a new block with the body of the if statement, then link that block
        // as a successor of the parent node, and a predecessor of the tail node
        lhs = lower_block(*stmt->ifblock.block, *this, state);
        IR::link_nodes(state.current_node, lhs);
        IR::link_nodes(lhs, tail);

        for (auto && elif : stmt->efblock) {
            // Create a new block that will be the other successor, this will
            // hold the condition of the `elif` branch, and then have it's own lhs for the body,
            // and a new rhs for additional `elif` or `else` blocks
            std::shared_ptr<IR::Node> rhs = std::make_shared<IR::Node>();
            IR::link_nodes(state.current_node, rhs, true);
            IR::link_nodes(rhs, tail);

            rhs->block->instructions.emplace_back(
                std::make_unique<IR::Instruction>(std::visit(el, elif.condition)));

            // Attach the body to this new lhs
            lhs = lower_block(*elif.block, *this, state);
            IR::link_nodes(state.current_node, lhs);
            IR::link_nodes(lhs, tail);

            // This is now the current node, as we build our if web
            state.current_node = rhs;
        }

        // Finally attach any else block. While this block may be empty, we'll
        // attach it anyway and allow any cleanup to be done later
        std::shared_ptr<IR::Node> rhs = lower_block(*stmt->eblock.block, *this, state);
        IR::link_nodes(state.current_node, rhs, true);
        IR::link_nodes(rhs, tail);

        // Return the tail, as this is now the only block we care about
        state.current_node = tail;
    }

    void operator()(const std::unique_ptr<AST::ForeachStatement> & stmt,
                    LoweringState & state) const {
        // A loop will end up being turned into at least 4 basic blocks
        //  1. A preamble which is used to force strictness, as well as set up
        //     and variables required before deconstructing the loop
        //  2. A header, this is where the condition of the loop is evaluated,
        //     and either continues to the body, or exits to the tail
        //  3. The body is the first block of the body of the loop. There may be
        //     additional successors to this block depending on the structure of
        //     the loop itself.
        //  4. The tail is the first block after the loop, it's the place that all
        //     exits to the loop will link to.
        //
        //
        //                          O preamble
        //                          |
        //                          O header
        //                         / \
        //                        |   O body
        //                         \ /
        //                          O tail

        // The preamble is used to initialize loop variables, of which there may be 1 or 2.
        // This ensures strictness
        auto preamble = std::make_shared<IR::Node>();
        IR::link_nodes(state.current_node, preamble);
        state.current_node = preamble;

        auto && id1 = std::make_shared<IR::Undefined>();
        preamble->block->instructions.emplace_back(
            std::make_unique<IR::Instruction>(std::move(id1), IR::Variable{stmt->id.value}));

        if (stmt->id2) {
            auto && id2 = std::make_shared<IR::Undefined>();
            preamble->block->instructions.emplace_back(std::make_unique<IR::Instruction>(
                std::move(id2), IR::Variable{stmt->id2.value().value}));
        }

        // This is the header where we evaluate the condition of the loop to decide if we will
        // continue or break
        state.loop_header = std::make_shared<IR::Node>();
        IR::link_nodes(state.current_node, state.loop_header);
        // TODO: we still need to:
        //  1. set the id (and id2 if necessary) to the next value on the array/dict
        //  2. check that we are at the end of the array
        //  3. do the appropriate thing based on that information.

        // This is the block that comes after the loop
        state.loop_tail = std::make_shared<IR::Node>();
        IR::link_nodes(state.loop_header, state.loop_tail, true);
        state.current_node = state.loop_header;

        // This is the first block of the body
        // We need to pass in a new state block, because we may have nested
        // loops, which will each need their own head/tail blocks.
        LoweringState lstate{state};
        auto lblock = lower_block(*stmt->block, *this, lstate);
        IR::link_nodes(state.current_node, lblock);

        state.current_node = state.loop_tail;
        state.loop_header = nullptr;
        state.loop_tail = nullptr;
    }

    void operator()(const std::unique_ptr<AST::Break> & stmt, LoweringState & state) const {
        // in the case of an `if ...: break` this will create an empty block,
        // that's okay we can clean it up later.
        assert(state.current_node->successors[0] == nullptr);
        IR::link_nodes(state.current_node, state.loop_tail);
    }

    void operator()(const std::unique_ptr<AST::Continue> & stmt, LoweringState & state) const {
        // in the case of an `if ...: continue` this will create an empty block,
        // that's okay we can clean it up later.
        assert(state.current_node->successors[0] == nullptr);
        IR::link_nodes(state.current_node, state.loop_header);
    }

  private:
    const ExpressionLowering el;
};

std::shared_ptr<IR::Node> lower_block(const AST::CodeBlock & block,
                                      const StatementLowering & lower) {
    auto root = std::make_shared<IR::Node>();
    LoweringState state{root};
    return lower_block(block, lower, state);
}

std::shared_ptr<IR::Node> lower_block(const AST::CodeBlock & block, const StatementLowering & lower,
                                      LoweringState & state) {
    auto root = state.current_node;
    for (auto && stmt : block.statements) {
        std::visit([&](auto && s) { lower(s, state); }, stmt);
    }
    return root;
}

} // namespace

IR::CFG ast_to_mir(const std::unique_ptr<Frontend::AST::CodeBlock> & block) {
    const StatementLowering lwr{};

    return IR::CFG{lower_block(*block, lwr)};
}

} // namespace MIR
