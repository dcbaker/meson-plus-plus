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

        return builder::make_instruction<IR::FunctionCall>(name, "meson++")
            .add_pos_arg(std::visit(*this, stmt->lhs))
            .add_pos_arg(std::visit(*this, stmt->lhs));
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

        return builder::make_instruction<IR::FunctionCall>(name, "meson++")
            .add_pos_arg(std::visit(*this, stmt->lhs))
            .add_pos_arg(std::visit(*this, stmt->lhs));
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

        return builder::make_instruction<IR::FunctionCall>(name, "meson++")
            .add_pos_arg(std::visit(*this, stmt->rhs));
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Number> & stmt) const {
        return std::make_shared<IR::Number>(stmt->value);
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::String> & stmt) const {
        return std::make_shared<IR::String>(stmt->value);
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Subscript> & stmt) const {
        return builder::make_instruction<IR::FunctionCall>("subscript", "meson++")
            .add_pos_arg(std::visit(*this, stmt->lhs))
            .add_pos_arg(std::visit(*this, stmt->lhs));
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

        return builder::make_instruction<IR::FunctionCall>(name, "meson++")
            .add_pos_arg(std::visit(*this, stmt->lhs))
            .add_pos_arg(std::visit(*this, stmt->lhs));
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::FunctionCall> & stmt) const {
        IR::InstructionType && fname = std::visit(*this, stmt->held);
        // TODO: error handling
        std::string name = std::get<std::shared_ptr<IR::String>>(fname)->m_value;
        auto f = builder::make_instruction<IR::FunctionCall>(name);

        for (auto && a : stmt->args->positional) {
            f.add_pos_arg(std::visit(*this, a));
        }

        IR::KeywordArguments && kws{};
        for (auto && [k, v] : stmt->args->keyword) {
            f.add_kw_arg(std::visit(*this, k), std::visit(*this, v));
        }

        return f;
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::GetAttribute> & stmt) const {
        return builder::make_instruction<IR::FunctionCall>("get_attribute"
                                                           "meson++")
            .add_pos_arg(std::visit(*this, stmt->holder))
            .add_pos_arg(std::visit(*this, stmt->held));
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Array> & stmt) const {
        auto arr = builder::make_instruction<IR::Array>();
        for (auto && v : stmt->elements) {
            arr.append(std::visit(*this, v));
        }
        return arr;
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Dict> & stmt) const {
        auto dict = builder::make_instruction<IR::Dict>();
        for (auto && [k, v] : stmt->elements) {
            dict.append(std::visit(*this, k), std::visit(*this, v));
        }

        return dict;
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Ternary> & stmt) const {
        return builder::make_instruction<IR::FunctionCall>("ternary"
                                                           "meson++")
            .add_pos_arg(std::visit(*this, stmt->condition))
            .add_pos_arg(std::visit(*this, stmt->lhs))
            .add_pos_arg(std::visit(*this, stmt->rhs));
    }
};

struct StatementLowering;

/// @brief State passed between StatementLowering calls
struct LoweringState {
    builder::Builder * current_node;
    builder::Builder * loop_header;
    builder::Builder * loop_tail;
};

std::shared_ptr<IR::Node> lower_block(const AST::CodeBlock & block, const StatementLowering & lower,
                                      LoweringState & state);

/// @brief Lower AST statements into MIR representations
struct StatementLowering {

    StatementLowering() : el{} {};

    StatementLowering(StatementLowering &&) = delete;
    StatementLowering & operator=(StatementLowering &&) = delete;

    void operator()(const std::unique_ptr<AST::Statement> & stmt, LoweringState & state) const {
        state.current_node->add_inst(std::make_unique<IR::Instruction>(std::visit(el, stmt->expr)));
    }

    void operator()(const std::unique_ptr<AST::Assignment> & stmt, LoweringState & state) const {
        IR::InstructionType lhs = std::visit(el, stmt->lhs);
        // TODO: error handling
        auto & id = std::get<std::shared_ptr<IR::Identifier>>(lhs);
        IR::InstructionType rhs = std::visit(el, stmt->rhs);

        // In Meson operators like x *= y are short for x = x * y
        // As such, MIR doesn't have representations for them, and they're easy to convert
        // At the AST -> MIR barrier
        std::string name;
        switch (stmt->op) {
            case AST::AssignOp::EQUAL:
                state.current_node->add_inst(
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

        state.current_node->add_inst(builder::make_instruction<IR::FunctionCall>(name, "meson++")
                                         .add_pos_arg(std::move(lhs))
                                         .add_pos_arg(std::move(rhs))
                                         .set_var(id->m_name));
    }

    void operator()(const std::unique_ptr<AST::IfStatement> & stmt, LoweringState & state) const {
        // This is the block that all of the branches of the if/elif/else web
        // will join back to
        builder::Builder tail{};

        // place the condition as the last instruction of the block.
        state.current_node->add_condition(
            std::make_unique<IR::Instruction>(std::visit(el, stmt->ifblock.condition)));

        // Create a new block of the left hand side. This block will be
        // connected to the current node on the lhs, and it will connect to the
        // tail on the left hand side.
        builder::Builder lhs{lower_block(*stmt->ifblock.block, *this, state)};
        state.current_node->link_left_successor(lhs);
        lhs.link_left_successor(tail);

        for (auto && elif : stmt->efblock) {
            // Create a new block that will be the other successor, this will
            // hold the condition of the `elif` branch, and then have it's own lhs for the body,
            // and a new rhs for additional `elif` or `else` blocks
            builder::Builder rhs = state.current_node->right_successor();
            rhs.add_condition(std::make_unique<IR::Instruction>(std::visit(el, elif.condition)));

            // Attach the body to this new lhs, following the same rules as for
            // the `if`
            lhs = lower_block(*elif.block, *this, state);
            rhs.link_left_successor(lhs);
            lhs.link_left_successor(tail);

            // This is now the current node, as we build our if web
            state.current_node = &rhs;
        }

        // Finally attach any else block. While this block may be empty, we'll
        // attach it anyway and allow any cleanup to be done later
        builder::Builder rhs{lower_block(*stmt->eblock.block, *this, state)};
        state.current_node->link_right_successor(rhs);
        rhs.link_left_successor(tail);

        // The tail is now the working block;
        state.current_node = &tail;
    }

    void operator()(const std::unique_ptr<AST::ForeachStatement> & stmt,
                    LoweringState & state) const {
        /* A loop will end up being turned into at least 4 basic blocks
         *  1. A preamble which is used to force strictness, as well as set up
         *     and variables required before deconstructing the loop
         *  2. A header, this is where the condition of the loop is evaluated,
         *     and either continues to the body, or exits to the tail
         *  3. The body is the first block of the body of the loop. There may be
         *     additional successors to this block depending on the structure of
         *     the loop itself.
         *  4. The tail is the first block after the loop, it's the place that all
         *     exits to the loop will link to.
         *
         *                          O preamble
         *                          |
         *                          O header
         *                         / \
         *                        |   O body
         *                         \ /
         *                          O tail
         *
         * The preamble is used to initialize loop variables, of which there may be 1 or 2.
         * This ensures strictness auto preamble = state.current_node->left_successor();
         */
        auto preamble = state.current_node->left_successor();
        preamble.add_inst(builder::make_instruction<IR::Undefined>().set_var(stmt->id.value));

        if (stmt->id2) {
            preamble.add_inst(
                builder::make_instruction<IR::Undefined>().set_var(stmt->id2.value().value));
            // TODO: call `.keys()` to get an array of keys, we can iterate that
            // We then do the same thing in both cases, index into the array,
            // set id1 = to array[index], then in the dict case we use the dict[key]
            // form to get the value.
        }
        // TODO: set a variable to the length of the array in both cases

        // This is the header where we evaluate the condition of the loop to decide if we will
        // continue or break
        auto header = preamble.left_successor();
        state.current_node = &header;

        // TODO: we still need to:
        //  1. set the id (and id2 if necessary) to the next value on the array/dict
        //  2. check that we are at the end of the array
        //  3. go back into the rhs block if we are not at the end of the array, or
        //     go to the tail if we are

        // This is the block that comes after the loop
        auto tail = state.current_node->left_successor();

        // This is the first block of the body
        // We need to pass in a new state block, because we may have nested
        // loops, which will each need their own head/tail blocks.
        auto rhs = state.current_node->right_successor();

        LoweringState lstate{
            .current_node = &rhs,
            .loop_header = &header,
            .loop_tail = &tail,
        };

        auto lblock = lower_block(*stmt->block, *this, lstate);
        header.link_right_successor(lblock);
        header.link_left_successor(header);

        state.current_node = &tail;
    }

    void operator()(const std::unique_ptr<AST::Break> & stmt, LoweringState & state) const {
        // in the case of an `if ...: break` this will create an empty block,
        // that's okay we can clean it up later.
        state.current_node->link_left_successor(*state.loop_tail);
    }

    void operator()(const std::unique_ptr<AST::Continue> & stmt, LoweringState & state) const {
        // in the case of an `if ...: continue` this will create an empty block,
        // that's okay we can clean it up later.
        state.current_node->link_left_successor(*state.loop_header);
    }

  private:
    const ExpressionLowering el;
};

std::shared_ptr<IR::Node> lower_block(const AST::CodeBlock & block,
                                      const StatementLowering & lower) {
    builder::Builder root{};
    LoweringState state{&root};
    return lower_block(block, lower, state);
}

std::shared_ptr<IR::Node> lower_block(const AST::CodeBlock & block, const StatementLowering & lower,
                                      LoweringState & state) {
    auto root = state.current_node;
    for (auto && stmt : block.statements) {
        std::visit([&](auto && s) { lower(s, state); }, stmt);
    }
    return root->get();
}

} // namespace

IR::CFG ast_to_mir(const std::unique_ptr<Frontend::AST::CodeBlock> & block) {
    const StatementLowering lwr{};

    return IR::CFG{lower_block(*block, lwr)};
}

} // namespace MIR
