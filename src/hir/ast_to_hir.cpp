// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "ast_to_hir.hpp"
#include "exceptions.hpp"

namespace HIR {

namespace {

struct StatementLowering {
    void operator()(const std::unique_ptr<Frontend::AST::Statement> & stmt){};
    void operator()(const std::unique_ptr<Frontend::AST::Assignment> & stmt){};
    void operator()(const std::unique_ptr<Frontend::AST::IfStatement> & stmt){};
    void operator()(const std::unique_ptr<Frontend::AST::ForeachStatement> & stmt){};
    void operator()(const std::unique_ptr<Frontend::AST::Break> & stmt){};
    void operator()(const std::unique_ptr<Frontend::AST::Continue> & stmt){};
};

struct ExpressionLowering {

    Object operator()(const std::unique_ptr<Frontend::AST::String> & expr) {
        return String{expr->value};
    };

    Object operator()(const std::unique_ptr<Frontend::AST::FunctionCall> & expr) {
        ExpressionLowering lower{};
        auto raw_fname = std::visit(lower, expr->id);
        auto fname_ptr = std::get_if<String>(&raw_fname);

        // This can be invalid, we could get basically anything here and need to
        // deal with it, but for now...
        if (fname_ptr == nullptr) {
            throw Util::Exceptions::InvalidArguments{"this might actually be valid"};
        }
        auto fname = fname_ptr->value;

        return FunctionCall{fname};
    };
    // XXX: all of thse are lies to get things compiling
    Object operator()(const std::unique_ptr<Frontend::AST::Boolean> & expr) {
        return String{"placeholder"};
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Identifier> & expr) {
        return String{"placeholder"};
    };
    Object operator()(const std::unique_ptr<Frontend::AST::AdditiveExpression> & expr) {
        return String{"placeholder"};
    };
    Object operator()(const std::unique_ptr<Frontend::AST::MultiplicativeExpression> & expr) {
        return String{"placeholder"};
    };
    Object operator()(const std::unique_ptr<Frontend::AST::UnaryExpression> & expr) {
        return String{"placeholder"};
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Number> & expr) {
        return String{"placeholder"};
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Subscript> & expr) {
        return String{"placeholder"};
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Relational> & expr) {
        return String{"placeholder"};
    };
    Object operator()(const std::unique_ptr<Frontend::AST::GetAttribute> & expr) {
        return String{"placeholder"};
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Array> & expr) {
        return String{"placeholder"};
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Dict> & expr) {
        return String{"placeholder"};
    };
    Object operator()(const std::unique_ptr<Frontend::AST::Ternary> & expr) {
        return String{"placeholder"};
    };
};

} // namespace

IRList lower_ast(const Frontend::AST::CodeBlock & block) {
    IRList bl{};
    StatementLowering lower{};

    // for (const auto & i : block.statements) {
    //     bl.emplace_back(std::visit(lower, i));
    // }

    return bl;
}

} // namespace HIR
