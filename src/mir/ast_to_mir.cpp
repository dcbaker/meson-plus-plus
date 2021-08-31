// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "ast_to_mir.hpp"
#include "exceptions.hpp"

namespace MIR {

namespace {

/**
 * Lowers AST expressions into MIR objects.
 */
struct ExpressionLowering {

    Object operator()(const std::unique_ptr<Frontend::AST::String> & expr) const {
        return std::make_unique<String>(expr->value);
    };

    Object operator()(const std::unique_ptr<Frontend::AST::FunctionCall> & expr) const {
        const ExpressionLowering lower{};

        // I think that a function can only be an ID, I think
        auto fname_id = std::visit(lower, expr->id);
        auto fname_ptr = std::get_if<std::unique_ptr<Identifier>>(&fname_id);
        if (fname_ptr == nullptr) {
            // TODO: Better error message witht the thing being called
            throw Util::Exceptions::MesonException{"Object is not callable"};
        }
        auto fname = (*fname_ptr)->value;

        // Get the positional arguments
        std::vector<Object> pos{};
        for (const auto & i : expr->args->positional) {
            pos.emplace_back(std::visit(lower, i));
        }

        std::unordered_map<std::string, Object> kwargs{};
        for (const auto & [k, v] : expr->args->keyword) {
            auto key_obj = std::visit(lower, k);
            auto key_ptr = std::get_if<std::unique_ptr<MIR::Identifier>>(&key_obj);
            if (key_ptr == nullptr) {
                // TODO: better error message
                throw Util::Exceptions::MesonException{"keyword arguments must be identifiers"};
            }
            auto key = (*key_ptr)->value;
            kwargs[key] = std::visit(lower, v);
        }

        // We have to move positional arguments because Object isn't copy-able
        return std::make_unique<FunctionCall>(fname, std::move(pos), std::move(kwargs));
    };

    Object operator()(const std::unique_ptr<Frontend::AST::Boolean> & expr) const {
        return std::make_unique<Boolean>(expr->value);
    };

    Object operator()(const std::unique_ptr<Frontend::AST::Number> & expr) const {
        return std::make_unique<Number>(expr->value);
    };

    Object operator()(const std::unique_ptr<Frontend::AST::Identifier> & expr) const {
        return std::make_unique<Identifier>(expr->value);
    };

    Object operator()(const std::unique_ptr<Frontend::AST::Array> & expr) const {
        const ExpressionLowering lower{};
        auto arr = std::make_unique<Array>();
        for (const auto & i : expr->elements) {
            arr->value.emplace_back(std::visit(lower, i));
        }
        return arr;
    };

    Object operator()(const std::unique_ptr<Frontend::AST::Dict> & expr) const {
        const ExpressionLowering lower{};
        auto dict = std::make_unique<Dict>();
        for (const auto & [k, v] : expr->elements) {
            auto key_obj = std::visit(lower, k);
            if (!std::holds_alternative<std::unique_ptr<String>>(key_obj)) {
                throw Util::Exceptions::InvalidArguments("Dictionary keys must be strintg");
            }
            auto key = std::get<std::unique_ptr<MIR::String>>(key_obj)->value;

            dict->value[key] = std::visit(lower, v);
        }
        return dict;
    };

    Object operator()(const std::unique_ptr<Frontend::AST::GetAttribute> & expr) const {
        // XXX: This is wrong, we can have things like:
        // meson.get_compiler('c').get_id()
        // Which this code *cannot* handle here.
        auto holding_obj = std::visit(*this, expr->object);
        assert(std::holds_alternative<std::unique_ptr<MIR::Identifier>>(holding_obj));

        // Meson only allows methods in objects, so we can enforce that this is a function
        auto method = std::visit(*this, expr->id);
        assert(std::holds_alternative<std::unique_ptr<MIR::FunctionCall>>(method));

        auto func = std::move(std::get<std::unique_ptr<MIR::FunctionCall>>(method));
        func->holder = std::get<std::unique_ptr<MIR::Identifier>>(holding_obj)->value;

        return func;
    };

    // XXX: all of thse are lies to get things compiling
    Object operator()(const std::unique_ptr<Frontend::AST::AdditiveExpression> & expr) const {
        return std::make_unique<String>("placeholder: add");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::MultiplicativeExpression> & expr) const {
        return std::make_unique<String>("placeholder: mul");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::UnaryExpression> & expr) const {
        return std::make_unique<String>("placeholder: unary");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Subscript> & expr) const {
        return std::make_unique<String>("placeholder: subscript");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Relational> & expr) const {
        return std::make_unique<String>("placeholder: rel");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Ternary> & expr) const {
        return std::make_unique<String>("placeholder: tern");
    };
};

/**
 * Lowers AST statements into MIR objects.
 */
struct StatementLowering {
    BasicBlock * operator()(BasicBlock * list,
                            const std::unique_ptr<Frontend::AST::Statement> & stmt) const {
        const ExpressionLowering l{};
        list->instructions.emplace_back(std::visit(l, stmt->expr));
        return list;
    };

    BasicBlock * operator()(BasicBlock * list,
                            const std::unique_ptr<Frontend::AST::IfStatement> & stmt) const {
        const StatementLowering s{};
        const ExpressionLowering l{};

        auto next_block = std::make_shared<BasicBlock>();

        assert(list != nullptr);
        auto cur = list;
        BasicBlock * last_block;

        cur->condition = std::optional<Condition>{std::visit(l, stmt->ifblock.condition)};
        for (const auto & i : stmt->ifblock.block->statements) {
            last_block = std::visit(
                [&](const auto & a) { return s(cur->condition.value().if_true.get(), a); }, i);
        }
        // We shouldn't have a condition here, this is where we wnat to put our next target
        assert(!last_block->condition.has_value());
        last_block->next = next_block;

        // We're building a web-like structure here, so we walk over the flat
        // list of elif conditions + statements, generating a web of BasicBlock
        // objects
        if (!stmt->efblock.empty()) {
            for (const auto & el : stmt->efblock) {
                cur = cur->condition->if_false.get();
                cur->condition = std::optional<Condition>{std::visit(l, el.condition)};
                for (const auto & i : el.block->statements) {
                    last_block = std::visit(
                        [&](const auto & a) { return s(cur->condition.value().if_true.get(), a); },
                        i);
                }

                // We shouldn't have a condition here, this is where we wnat to put our next target
                assert(!last_block->condition.has_value());
                last_block->next = next_block;
            }
        }

        if (stmt->eblock.block != nullptr) {
            for (const auto & i : stmt->eblock.block->statements) {
                last_block = std::visit(
                    [&](const auto & a) { return s(cur->condition.value().if_false.get(), a); }, i);
            }
        }
        // We shouldn't have a condition here, this is where we wnat to put our next target
        assert(!last_block->condition.has_value());
        last_block->next = next_block;

        // Return the raw pointer, which is fine because we're not giving the
        // caller ownership of the pointer, the other basic blocks are the owners.
        return next_block.get();
    };

    BasicBlock * operator()(BasicBlock * list,
                            const std::unique_ptr<Frontend::AST::Assignment> & stmt) const {
        const ExpressionLowering l{};
        auto target = std::visit(l, stmt->lhs);
        auto value = std::visit(l, stmt->rhs);

        // XXX: need to handle mutative assignments
        assert(stmt->op == Frontend::AST::AssignOp::EQUAL);

        // XXX: need to handle other things that can be assigned to, like subscript
        auto name_ptr = std::get_if<std::unique_ptr<Identifier>>(&target);
        if (name_ptr == nullptr) {
            throw Util::Exceptions::MesonException{
                "This might be a bug, or might be an incomplete implementation"};
        }
        std::visit([&](const auto & t) { t->var.name = (*name_ptr)->value; }, value);

        list->instructions.emplace_back(std::move(value));
        return list;
    };

    // XXX: None of this is actually implemented
    BasicBlock * operator()(BasicBlock * list,
                            const std::unique_ptr<Frontend::AST::ForeachStatement> & stmt) const {
        return list;
    };
    BasicBlock * operator()(BasicBlock * list,
                            const std::unique_ptr<Frontend::AST::Break> & stmt) const {
        return list;
    };
    BasicBlock * operator()(BasicBlock * list,
                            const std::unique_ptr<Frontend::AST::Continue> & stmt) const {
        return list;
    };
};

} // namespace

/**
 * Lower AST representation into MIR.
 */
BasicBlock lower_ast(const std::unique_ptr<Frontend::AST::CodeBlock> & block) {
    BasicBlock bl{};
    BasicBlock * current_block = &bl;
    const StatementLowering lower{};
    for (const auto & i : block->statements) {
        current_block = std::visit([&](const auto & a) { return lower(current_block, a); }, i);
    }

    return bl;
}

} // namespace MIR
