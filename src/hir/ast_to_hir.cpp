// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "ast_to_hir.hpp"
#include "exceptions.hpp"

namespace HIR {

namespace {

struct ExpressionLowering {

    Object operator()(const std::unique_ptr<Frontend::AST::String> & expr) const {
        return std::make_unique<String>(expr->value);
    };

    Object operator()(const std::unique_ptr<Frontend::AST::FunctionCall> & expr) const {
        ExpressionLowering lower{};
        auto fname = std::visit(lower, expr->id);
        return std::make_unique<FunctionCall>(std::move(fname));
    };

    Object operator()(const std::unique_ptr<Frontend::AST::Boolean> & expr) const {
        return std::make_unique<Boolean>(expr->value);
    };

    Object operator()(const std::unique_ptr<Frontend::AST::Number> & expr) const {
        return std::make_unique<Number>(expr->value);
    };

    // XXX: all of thse are lies to get things compiling
    Object operator()(const std::unique_ptr<Frontend::AST::Identifier> & expr) const {
        return std::make_unique<String>("placeholder: id");
    };
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
    Object operator()(const std::unique_ptr<Frontend::AST::Array> & expr) const {
        return std::make_unique<String>("placeholder: arr");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Dict> & expr) const {
        return std::make_unique<String>("placeholder: dict");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Ternary> & expr) const {
        return std::make_unique<String>("placeholder: tern");
    };
};

struct StatementLowering {
    Object operator()(const std::unique_ptr<Frontend::AST::Statement> & stmt) const {
        ExpressionLowering l{};
        return std::visit(l, stmt->expr);
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Assignment> & stmt) const {
        return std::make_unique<String>("placeholder: assign");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::IfStatement> & stmt) const {
        return std::make_unique<String>("placeholder: if");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::ForeachStatement> & stmt) const {
        return std::make_unique<String>("placeholder: foreach");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Break> & stmt) const {
        return std::make_unique<String>("placeholder: break");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Continue> & stmt) const {
        return std::make_unique<String>("placeholder: continue");
    };
};

} // namespace

IRList lower_ast(const std::unique_ptr<Frontend::AST::CodeBlock> & block) {
    IRList bl{};
    StatementLowering lower{};

    for (const auto & i : block->statements) {
        bl.emplace_back(std::visit(lower, i));
    }

    return bl;
}

} // namespace HIR
