// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "ast_to_mir.hpp"
#include "builder/builder.hpp"
#include "exceptions.hpp"
#include "ir/instruction.hpp"
#include "passes/remove_ternary.hpp"

#include <memory>
#include <stdexcept>

namespace MIR {

namespace {

using namespace Frontend;

/// @brief Lower AST expressions into MIR representations
struct ExpressionLowering {
    IR::InstructionType operator()(const std::unique_ptr<AST::AdditiveExpression> & stmt) const {
        std::string name;
        IR::FunctionId fid;
        switch (stmt->op) {
            case AST::AddOp::ADD:
                name = "addition";
                fid = IR::FunctionId::addition;
                break;
            case AST::AddOp::SUB:
                name = "subtraction";
                fid = IR::FunctionId::subtraction;
                break;
            default:
                throw std::runtime_error{"Unknown additive expression type"};
        }

        return builder::make_instruction<IR::FunctionCall>(name, "meson++", fid)
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
        IR::FunctionId fid;
        switch (stmt->op) {
            case AST::MulOp::MOD:
                name = "modulo";
                fid = IR::FunctionId::modulo;
                break;
            case AST::MulOp::MUL:
                name = "multiplication";
                fid = IR::FunctionId::multiplication;
                break;
            case AST::MulOp::DIV:
                name = "division";
                fid = IR::FunctionId::division;
                break;
            default:
                throw std::runtime_error{"Unknown multiplication expression type"};
        }

        return builder::make_instruction<IR::FunctionCall>(name, "meson++", fid)
            .add_pos_arg(std::visit(*this, stmt->lhs))
            .add_pos_arg(std::visit(*this, stmt->lhs));
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::UnaryExpression> & stmt) const {
        std::string name;
        IR::FunctionId fid;
        switch (stmt->op) {
            case AST::UnaryOp::NEG:
                name = "negate";
                fid = IR::FunctionId::negate;
                break;
            case AST::UnaryOp::NOT:
                name = "logical_not";
                fid = IR::FunctionId::logical_not;
                break;
            default:
                throw std::runtime_error{"Unknown unary expression type"};
        }

        return builder::make_instruction<IR::FunctionCall>(name, "meson++", fid)
            .add_pos_arg(std::visit(*this, stmt->rhs));
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Number> & stmt) const {
        return std::make_shared<IR::Number>(stmt->value);
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::String> & stmt) const {
        return std::make_shared<IR::String>(stmt->value);
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Subscript> & stmt) const {
        return builder::make_instruction<IR::FunctionCall>("subscript", "meson++",
                                                           IR::FunctionId::subscript)
            .add_pos_arg(std::visit(*this, stmt->lhs))
            .add_pos_arg(std::visit(*this, stmt->lhs));
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::Relational> & stmt) const {
        IR::FunctionId fid;
        std::string name;
        bool negate = false;
        switch (stmt->op) {
            case AST::RelationalOp::AND:
                name = "logical_and";
                fid = IR::FunctionId::logical_and;
                break;
            case AST::RelationalOp::OR:
                name = "logical_or";
                fid = IR::FunctionId::logical_or;
                break;
            case AST::RelationalOp::NE:
                negate = true;
                [[fallthrough]];
            case AST::RelationalOp::EQ:
                name = "equal";
                fid = IR::FunctionId::equal;
                break;
            case AST::RelationalOp::GE:
                name = "greater_equal";
                fid = IR::FunctionId::greater_equal;
                break;
            case AST::RelationalOp::GT:
                name = "greater_than";
                fid = IR::FunctionId::greater_than;
                break;
            case AST::RelationalOp::LT:
                name = "less_than";
                fid = IR::FunctionId::less_than;
                break;
            case AST::RelationalOp::LE:
                name = "less_equal";
                fid = IR::FunctionId::less_equal;
                break;
            case AST::RelationalOp::NOT_IN:
                negate = true;
                [[fallthrough]];
            case AST::RelationalOp::IN:
                name = "in";
                fid = IR::FunctionId::contains;
                break;
            default:
                throw std::runtime_error{"Unknown relation expression type"};
        }

        auto b = builder::make_instruction<IR::FunctionCall>(name, "meson++", fid)
                     .add_pos_arg(std::visit(*this, stmt->lhs))
                     .add_pos_arg(std::visit(*this, stmt->lhs))
                     .as_type();

        // for "x != y" and "x not in y", we can rewrite that as "not(equal(x,
        // y))" and "not(contains(x, y))", which saves us on opcodes
        if (negate) {
            return builder::make_instruction<IR::FunctionCall>("not", "meson++",
                                                               IR::FunctionId::logical_not)
                .add_pos_arg(std::move(b));
        }
        return b;
    }

    IR::InstructionType operator()(const std::unique_ptr<AST::FunctionCall> & stmt) const {
        IR::InstructionType && fname = std::visit(*this, stmt->held);
        if (!std::get<std::shared_ptr<IR::Identifier>>(fname)) {
            throw Util::Exceptions::MesonException{"function name does not hold an identifier"};
        }
        std::string name = std::get<std::shared_ptr<IR::Identifier>>(fname)->m_name;
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
        return builder::make_instruction<IR::FunctionCall>("get_attribute", "meson++",
                                                           IR::FunctionId::get_attribute)
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
        return builder::make_instruction<IR::FunctionCall>("ternary", "meson++",
                                                           IR::FunctionId::ternary)
            .add_pos_arg(std::visit(*this, stmt->condition))
            .add_pos_arg(std::visit(*this, stmt->lhs))
            .add_pos_arg(std::visit(*this, stmt->rhs));
    }
};

struct StatementLowering;

/// @brief State passed between StatementLowering calls
struct LoweringState {
    std::shared_ptr<builder::Builder> current_node;
    std::shared_ptr<builder::Builder> loop_header;
    std::shared_ptr<builder::Builder> loop_tail;
    std::shared_ptr<uint64_t> m_tmp_var;

    std::string tmp_var() { return "mesonpp_tmp_" + std::to_string((*m_tmp_var)++); }
    std::string tmp_var(std::string name) {
        return "mesonpp_tmp_" + name + "_" + std::to_string((*m_tmp_var)++);
    }
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
        IR::FunctionId fid;
        switch (stmt->op) {
            case AST::AssignOp::EQUAL:
                state.current_node->add_inst(
                    std::make_unique<IR::Instruction>(std::move(rhs), IR::Variable{id->m_name}));
                return;
            case AST::AssignOp::ADD_EQUAL:
                name = "addition";
                fid = IR::FunctionId::addition;
                break;
            case AST::AssignOp::SUB_EQUAL:
                name = "subtraction";
                fid = IR::FunctionId::subtraction;
                break;
            case AST::AssignOp::DIV_EQUAL:
                name = "division";
                fid = IR::FunctionId::division;
                break;
            case AST::AssignOp::MUL_EQUAL:
                name = "multiplication";
                fid = IR::FunctionId::multiplication;
                break;
            case AST::AssignOp::MOD_EQUAL:
                name = "modulo";
                fid = IR::FunctionId::modulo;
                break;
            default:
                throw std::runtime_error{"Unknown operator"};
        }

        state.current_node->add_inst(
            builder::make_instruction<IR::FunctionCall>(name, "meson++", fid)
                .add_pos_arg(std::move(lhs))
                .add_pos_arg(std::move(rhs))
                .set_var(id->m_name));
    }

    void operator()(const std::unique_ptr<AST::IfStatement> & stmt, LoweringState & state) const {
        /* The left leg of a condition is always the "main" successor that means
         * if there is only one successor, it is the left. That also means that if
         * the condition is true we exit to the right.
         *
         * Thus a if/elif/else block will look like:
         *
         *                        O₁
         *                       / \
         *                      O₂  O₃
         *                      |  / \
         *                      | O₄  O₅
         *                      |/   / \
         *                       \  O₆  O₇
         *                        \/   /
         *                         \  /
         *                          \/
         *                           O₈
         *
         *
         * 1 previous block
         * 2 body of if
         * 3 condition for first elif
         * 4 body of first elif
         * 5 condition of additional elif...
         * 6 body of first elif...
         * 7 body of else
         * 8 tail block
         */

        // place the condition as the last instruction of the block.
        state.current_node->add_condition(
            std::make_unique<IR::Instruction>(std::visit(el, stmt->ifblock.condition)));

        // This is the block that all of the branches of the if/elif/else web
        // will join back to
        auto tail = std::make_shared<builder::Builder>();

        // Create a new block of the left hand side. This block will be
        // connected to the current node on the lhs, and it will connect to the
        // tail on the left hand side.
        auto lhs = std::make_shared<builder::Builder>(state.current_node->left_successor());
        lhs->link_left_successor(tail);

        // Use a new state with the block we created
        LoweringState lstate{state};
        lstate.current_node = lhs;
        lower_block(*stmt->ifblock.block, *this, lstate);

        for (auto && elif : stmt->efblock) {
            // Create a new block that will be the other successor, this will
            // hold the condition of the `elif` branch, and then have it's own lhs for the body,
            // and a new rhs for additional `elif` or `else` blocks
            auto rhs = std::make_shared<builder::Builder>(state.current_node->right_successor());
            rhs->add_condition(std::make_unique<IR::Instruction>(std::visit(el, elif.condition)));

            // This is the body of the elif
            auto lhs = std::make_shared<builder::Builder>(rhs->left_successor());

            lstate = {state};
            lstate.current_node = lhs;
            lower_block(*elif.block, *this, lstate);

            // This is now the current node, as we build our if web
            state.current_node = rhs;
        }

        if (stmt->eblock.block) {
            // Finally attach any else block.
            auto rhs = std::make_shared<builder::Builder>(state.current_node->right_successor());
            rhs->link_left_successor(tail);
            lstate = {state};
            lstate.current_node = rhs;
            lower_block(*stmt->eblock.block, *this, lstate);
        } else {
            state.current_node->link_right_successor(tail);
        }

        // The tail is now the working block;
        state.current_node = tail;
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

        const std::string array = state.tmp_var("loop_array");
        const std::string dict = state.tmp_var("loop_dict");
        const std::string cursor = state.tmp_var("loop_container_cursor");
        const std::string container_size = state.tmp_var("loop_container_size");

        builder::Builder preamble = state.current_node->left_successor();
        preamble.add_inst(builder::make_instruction<IR::Undefined>().set_var(stmt->id.value));

        if (stmt->id2) {
            // call `.keys()` to get an array of keys, we can iterate that
            // We then do the same thing in both cases, index into the array,
            // set id1 = to array[index], then in the dict case we use the dict[key]
            // form to get the value.
            preamble
                .add_inst(
                    builder::make_instruction<IR::Undefined>().set_var(stmt->id2.value().value))
                .add_inst(std::make_unique<IR::Instruction>(std::visit(el, stmt->expr),
                                                            IR::Variable{dict}))
                .add_inst(builder::make_instruction<IR::FunctionCall>(
                              "keys", builder::make_instruction<IR::Identifier>(dict),
                              IR::FunctionId::dict_keys)
                              .set_var(array));
        } else {
            preamble.add_inst(
                std::make_unique<IR::Instruction>(std::visit(el, stmt->expr), IR::Variable{array}));
        }

        // Find the length of the container, as well as set the default value for the cursor
        preamble
            .add_inst(builder::make_instruction<IR::FunctionCall>(
                          "length", builder::make_instruction<IR::Identifier>(array),
                          IR::FunctionId::array_length)
                          .set_var(container_size))
            .add_inst(builder::make_instruction<IR::Number>(0).set_var(cursor));

        // This is the header where we evaluate the condition of the loop to decide if we will
        // continue or break
        state.current_node = std::make_shared<builder::Builder>(preamble.left_successor());
        auto header = *state.current_node;

        const std::string loop_condition = state.tmp_var("loop_condition");

        // If the cursor is the same size as the array, we've read to the end
        // and it's time to break, otherwise we can go ahead to the loop body
        header.add_inst(builder::make_instruction<IR::FunctionCall>("subscript", "meson++",
                                                                    IR::FunctionId::subscript)
                            .add_pos_arg(builder::make_instruction<IR::Identifier>(array))
                            .add_pos_arg(builder::make_instruction<IR::Identifier>(cursor))
                            .set_var(stmt->id.value));

        // For dictionaries set the the second reference to be the dictionary value
        if (stmt->id2) {
            header.add_inst(
                builder::make_instruction<IR::FunctionCall>(
                    "get", builder::make_instruction<IR::Identifier>(dict),
                    IR::FunctionId::dict_get)
                    .add_pos_arg(builder::make_instruction<IR::Identifier>(stmt->id.value))
                    .set_var(stmt->id2.value().value));
        }

        // Set the condition of the block to check if the cursor is equal to the
        // size of the array, as that means it has read off the end. Increment the cursor after
        // checking the condition
        header
            .add_inst(builder::make_instruction<IR::FunctionCall>("equal", "meson++",
                                                                  IR::FunctionId::equal)
                          .add_pos_arg(builder::make_instruction<IR::Identifier>(cursor))
                          .add_pos_arg(builder::make_instruction<IR::Identifier>(container_size))
                          .set_var(loop_condition))
            .add_inst(builder::make_instruction<IR::FunctionCall>("addition", "meson++",
                                                                  IR::FunctionId::addition)
                          .add_pos_arg(builder::make_instruction<IR::Identifier>("cursor"))
                          .add_pos_arg(builder::make_instruction<IR::Number>(1))
                          .set_var(cursor))
            .add_condition(builder::make_instruction<IR::Identifier>(loop_condition))
            .set_loop_header();

        // This is the block that comes after the loop
        auto tail = std::make_shared<builder::Builder>(state.current_node->right_successor());

        // This is the first block of the body
        // We need to pass in a new state block, because we may have nested
        // loops, which will each need their own head/tail blocks.
        auto body = std::make_shared<builder::Builder>(state.current_node->left_successor());

        LoweringState lstate{
            .current_node = body,
            .loop_header = state.current_node,
            .loop_tail = tail,
            .m_tmp_var = state.m_tmp_var,
        };
        lower_block(*stmt->block, *this, lstate);
        lstate.current_node->link_left_successor(header);

        state.current_node = tail;
    }

    void operator()(const std::unique_ptr<AST::Break> & stmt, LoweringState & state) const {
        // in the case of an `if ...: break` this will create an empty block,
        // that's okay we can clean it up later.
        state.current_node->link_left_successor(state.loop_tail);
    }

    void operator()(const std::unique_ptr<AST::Continue> & stmt, LoweringState & state) const {
        // in the case of an `if ...: continue` this will create an empty block,
        // that's okay we can clean it up later.
        state.current_node->link_left_successor(state.loop_header);
    }

  private:
    const ExpressionLowering el;
};

std::shared_ptr<IR::Node> lower_block(const AST::CodeBlock & block,
                                      const StatementLowering & lower) {
    LoweringState state{
        std::make_shared<builder::Builder>(),
        nullptr,
        nullptr,
        std::make_shared<uint64_t>(0),
    };
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

    IR::CFG cfg{lower_block(*block, lwr)};

    bool progress;
    do {
        progress = false;
        for (auto n : cfg) {
            progress |= Passes::remove_ternary(n);
        }
    } while (progress);

    return cfg;
}

} // namespace MIR
