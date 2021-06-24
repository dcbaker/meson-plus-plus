// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "ast_to_hir.hpp"
#include "exceptions.hpp"

namespace HIR {

namespace {

/**
 * Lowers AST expressions into HIR objects.
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
            // Better error message witht the thing being called
            throw Util::Exceptions::MesonException{"Object is not callable"};
        }
        auto fname = (*fname_ptr)->value;

        return std::make_unique<FunctionCall>(std::move(fname));
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
            auto key = std::get<std::unique_ptr<HIR::String>>(key_obj)->value;

            dict->value[key] = std::visit(lower, v);
        }
        return dict;
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
    Object operator()(const std::unique_ptr<Frontend::AST::GetAttribute> & expr) const {
        return std::make_unique<String>("placeholder: getattr");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Ternary> & expr) const {
        return std::make_unique<String>("placeholder: tern");
    };
};

/**
 * Lowers AST statements into HIR objects.
 */
struct StatementLowering {
    void operator()(IRList * list, const std::unique_ptr<Frontend::AST::Statement> & stmt) const {
        const ExpressionLowering l{};
        list->instructions.emplace_back(std::visit(l, stmt->expr));
    };

    void operator()(IRList * list, const std::unique_ptr<Frontend::AST::IfStatement> & stmt) const {
        const StatementLowering s{};
        const ExpressionLowering l{};

        assert(list != nullptr);
        auto cur = list;

        cur->condition = std::optional<Condition>{std::visit(l, stmt->ifblock.condition)};
        for (const auto & i : stmt->ifblock.block->statements) {
            std::visit([&](const auto & a) { s(list->condition.value().if_true.get(), a); }, i);
        }

        // We're building a web-like structure here, so we walk over the flat
        // list of elif conditions + statements, generating a web of IRList
        // objects
        if (!stmt->efblock.empty()) {
            for (const auto & el : stmt->efblock) {
                cur = cur->condition->if_false.get();
                cur->condition = std::optional<Condition>{std::visit(l, el.condition)};
                for (const auto & i : el.block->statements) {
                    std::visit([&](const auto & a) { s(cur->condition.value().if_true.get(), a); },
                               i);
                }
            }
        }

        if (stmt->eblock.block != nullptr) {
            for (const auto & i : stmt->eblock.block->statements) {
                std::visit([&](const auto & a) { s(cur->condition.value().if_false.get(), a); }, i);
            }
        }
    };

    void operator()(IRList * list, const std::unique_ptr<Frontend::AST::Assignment> & stmt) const {
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
    };

    // XXX: None of this is actually implemented
    void operator()(IRList * list,
                    const std::unique_ptr<Frontend::AST::ForeachStatement> & stmt) const {};
    void operator()(IRList * list, const std::unique_ptr<Frontend::AST::Break> & stmt) const {};
    void operator()(IRList * list, const std::unique_ptr<Frontend::AST::Continue> & stmt) const {};
};

} // namespace

/**
 * Lower AST representation into HIR.
 */
IRList lower_ast(const std::unique_ptr<Frontend::AST::CodeBlock> & block) {
    IRList bl{};
    const StatementLowering lower{};

    for (const auto & i : block->statements) {
        std::visit([&](const auto & a) { lower(&bl, a); }, i);
    }

    return bl;
}

} // namespace HIR
