// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "ast_to_mir.hpp"
#include "builder/builder.hpp"
#include "ir/instruction.hpp"

#include <memory>
#include <stdexcept>

namespace MIR {

namespace {

using namespace Frontend;

/// @brief Lower AST expressions into MIR representations
struct ExpressionLowering {
    IR::InstructionType operator()(const std::unique_ptr<AST::AdditiveExpression> & stmt) const {
        std::string name;
        switch (stmt->op) {
            case AST::AddOp::ADD:
                name = "addition";
                break;
            case AST::AddOp::SUB:
                name = "subtraction";
                break;
            default:
                throw std::runtime_error{"Unknown additive expression type"};
        }

        return Builder::Builder{}
            .new_inst<IR::FunctionCall>(name, "meson++")
            .add_pos_arg(std::visit(*this, stmt->lhs))
            .add_pos_arg(std::visit(*this, stmt->lhs))
            .as_type();
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Boolean> & stmt) const {
        return std::make_shared<IR::Boolean>(stmt->value);
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Identifier> & stmt) const {
        return std::make_shared<IR::Identifier>(stmt->value);
    }

    IR::InstructionType
    operator()(const std::unique_ptr<AST::MultiplicativeExpression> & stmt) const {
        std::string name;
        switch (stmt->op) {
            case AST::MulOp::MOD:
                name = "modulo";
                break;
            case AST::MulOp::MUL:
                name = "multiplication";
                break;
            case AST::MulOp::DIV:
                name = "division";
                break;
            default:
                throw std::runtime_error{"Unknown multiplication expression type"};
        }

        return Builder::Builder{}
            .new_inst<IR::FunctionCall>(name, "meson++")
            .add_pos_arg(std::visit(*this, stmt->lhs))
            .add_pos_arg(std::visit(*this, stmt->lhs))
            .as_type();
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::UnaryExpression> & stmt) const {
        std::string name;
        switch (stmt->op) {
            case AST::UnaryOp::NEG:
                name = "negate";
                break;
            case AST::UnaryOp::NOT:
                name = "logical_not";
                break;
            default:
                throw std::runtime_error{"Unknown unary expression type"};
        }

        return Builder::Builder{}
            .new_inst<IR::FunctionCall>(name, "meson++")
            .add_pos_arg(std::visit(*this, stmt->rhs))
            .as_type();
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Number> & stmt) const {
        return std::make_shared<IR::Number>(stmt->value);
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::String> & stmt) const {
        return std::make_shared<IR::String>(stmt->value);
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Subscript> & stmt) const {
        return Builder::Builder{}
            .new_inst<IR::FunctionCall>("subscript", "meson++")
            .add_pos_arg(std::visit(*this, stmt->lhs))
            .add_pos_arg(std::visit(*this, stmt->lhs))
            .as_type();
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Relational> & stmt) const {
        // TODO: we could rewrite not_in and not_equal as not(in) and not(equal), respectively
        // This would save us on
        std::string name;
        switch (stmt->op) {
            case AST::RelationalOp::AND:
                name = "logical_and";
                break;
            case AST::RelationalOp::OR:
                name = "logical_or";
                break;
            case AST::RelationalOp::EQ:
                name = "equal";
                break;
            case AST::RelationalOp::NE:
                name = "not_equal";
                break;
            case AST::RelationalOp::GE:
                name = "greater_equal";
                break;
            case AST::RelationalOp::GT:
                name = "greater_than";
                break;
            case AST::RelationalOp::LT:
                name = "less_than";
                break;
            case AST::RelationalOp::LE:
                name = "less_equal";
                break;
            case AST::RelationalOp::NOT_IN:
                name = "not_in";
                break;
            case AST::RelationalOp::IN:
                name = "in";
                break;
            default:
                throw std::runtime_error{"Unknown relation expression type"};
        }

        return Builder::Builder{}
            .new_inst<IR::FunctionCall>(name, "meson++")
            .add_pos_arg(std::visit(*this, stmt->lhs))
            .add_pos_arg(std::visit(*this, stmt->lhs))
            .as_type();
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::FunctionCall> & stmt) const {

        IR::InstructionType && fname = std::visit(*this, stmt->held);
        // TODO: error handling
        std::string name = std::get<std::shared_ptr<IR::String>>(fname)->m_value;
        auto f = Builder::Builder{}.new_inst<IR::FunctionCall>(name);

        for (auto && a : stmt->args->positional) {
            f.add_pos_arg(std::visit(*this, a));
        }

        IR::KeywordArguments && kws{};
        for (auto && [k, v] : stmt->args->keyword) {
            f.add_kw_arg(std::visit(*this, k), std::visit(*this, v));
        }

        return f.as_type();
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::GetAttribute> & stmt) const {
        return Builder::Builder{}
            .new_inst<IR::FunctionCall>("get_attribute"
                                        "meson++")
            .add_pos_arg(std::visit(*this, stmt->holder))
            .add_pos_arg(std::visit(*this, stmt->held))
            .as_type();
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Array> & stmt) const {
        auto arr = Builder::Builder{}.new_inst<IR::Array>();
        for (auto && v : stmt->elements) {
            arr.append(std::visit(*this, v));
        }
        return arr.as_type();
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Dict> & stmt) const {
        auto dict = Builder::Builder{}.new_inst<IR::Dict>();
        for (auto && [k, v] : stmt->elements) {
            dict.append(std::visit(*this, k), std::visit(*this, v));
        }

        return dict.as_type();
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Ternary> & stmt) const {
        return Builder::Builder{}
            .new_inst<IR::FunctionCall>("ternary"
                                        "meson++")
            .add_pos_arg(std::visit(*this, stmt->condition))
            .add_pos_arg(std::visit(*this, stmt->lhs))
            .add_pos_arg(std::visit(*this, stmt->rhs))
            .as_type();
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
        IR::InstructionType rhs = std::visit(el, stmt->rhs);

        Builder::Builder b{};

        // In Meson operators like x *= y are short for x = x * y
        // As such, MIR doesn't have representations for them, and they're easy to convert
        // At the AST -> MIR barrier
        std::string name;
        switch (stmt->op) {
            case AST::AssignOp::EQUAL:
                state.current_node->block->instructions.emplace_back(
                    std::make_unique<IR::Instruction>(std::move(rhs), IR::Variable{id->m_name}));
                return;
            case AST::AssignOp::ADD_EQUAL:
                name = "addition";
                break;
            case AST::AssignOp::SUB_EQUAL:
                name = "subtraction";
                break;
            case AST::AssignOp::DIV_EQUAL:
                name = "division";
                break;
            case AST::AssignOp::MUL_EQUAL:
                name = "multiplication";
                break;
            case AST::AssignOp::MOD_EQUAL:
                name = "modulo";
                break;
            default:
                throw std::runtime_error{"Unknown operator"};
        }

        state.current_node->block->instructions.emplace_back(
            b.new_inst<IR::FunctionCall>(name, "meson++")
                .add_pos_arg(std::move(lhs))
                .add_pos_arg(std::move(rhs))
                .set_var(id->m_name)
                .as_instr());
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
