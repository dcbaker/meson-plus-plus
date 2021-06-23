// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "ast_to_hir.hpp"
#include "exceptions.hpp"

namespace HIR {

namespace {

struct StatementLowering {
    Object operator()(const std::unique_ptr<Frontend::AST::Statement> & stmt) {
        return std::make_unique<String>("placeholder");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Assignment> & stmt) {
        return std::make_unique<String>("placeholder");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::IfStatement> & stmt) {
        return std::make_unique<String>("placeholder");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::ForeachStatement> & stmt) {
        return std::make_unique<String>("placeholder");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Break> & stmt) {
        return std::make_unique<String>("placeholder");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Continue> & stmt) {
        return std::make_unique<String>("placeholder");
    };
};

struct ExpressionLowering {

    Object operator()(const std::unique_ptr<Frontend::AST::String> & expr) {
        return std::make_unique<String>(expr->value);
    };

    Object operator()(const std::unique_ptr<Frontend::AST::FunctionCall> & expr) {
        ExpressionLowering lower{};
        auto fname = std::visit(lower, expr->id);
        return std::make_unique<FunctionCall>(std::move(fname));
    };

    // XXX: all of thse are lies to get things compiling
    Object operator()(const std::unique_ptr<Frontend::AST::Boolean> & expr) {
        return std::make_unique<String>("placeholder");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Identifier> & expr) {
        return std::make_unique<String>("placeholder");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::AdditiveExpression> & expr) {
        return std::make_unique<String>("placeholder");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::MultiplicativeExpression> & expr) {
        return std::make_unique<String>("placeholder");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::UnaryExpression> & expr) {
        return std::make_unique<String>("placeholder");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Number> & expr) {
        return std::make_unique<String>("placeholder");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Subscript> & expr) {
        return std::make_unique<String>("placeholder");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Relational> & expr) {
        return std::make_unique<String>("placeholder");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::GetAttribute> & expr) {
        return std::make_unique<String>("placeholder");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Array> & expr) {
        return std::make_unique<String>("placeholder");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Dict> & expr) {
        return std::make_unique<String>("placeholder");
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Ternary> & expr) {
        return std::make_unique<String>("placeholder");
    };
};

} // namespace

IRList lower_ast(const Frontend::AST::CodeBlock & block) {
    IRList bl{};
    StatementLowering lower{};

    for (const auto & i : block.statements) {
        bl.emplace_back(std::visit(lower, i));
    }

    return bl;
}

} // namespace HIR
