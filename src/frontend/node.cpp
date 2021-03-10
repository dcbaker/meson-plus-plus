// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <numeric>

#include "node.hpp"
#include "node_visitors.hpp"

namespace Frontend::AST {

namespace {

struct AsStringVisitor {
    std::string operator()(const std::unique_ptr<String> & s) {
        return s->as_string();
    };

    std::string operator()(const std::unique_ptr<Number> & s) {
        return s->as_string();
    }

    std::string operator()(const std::unique_ptr<Identifier> & s) {
        return s->as_string();
    }

    std::string operator()(const std::unique_ptr<Boolean> & s) {
        return s->as_string();
    }

    std::string operator()(const std::unique_ptr<UnaryExpression> & s) {
        // There's currently only unary negation
        return "-" + std::visit(AsStringVisitor(), s->rhs);
    }

    std::string operator()(const std::unique_ptr<AdditiveExpression> & s) {
        std::string o;
        switch (s->op) {
            case AddOp::ADD:
                o = "+";
                break;
            case AddOp::SUB:
                o = "-";
                break;
        }

        return std::visit(AsStringVisitor(), s->lhs) + " " + o + " " + std::visit(AsStringVisitor(), s->rhs);
    }

    std::string operator()(const std::unique_ptr<MultiplicativeExpression> & s) {
        std::string o;
        switch (s->op) {
            case MulOp::MOD:
                o = "%";
                break;
            case MulOp::MUL:
                o = "*";
                break;
            case MulOp::DIV:
                o = "/";
                break;
        }

        return std::visit(AsStringVisitor(), s->lhs) + " " + o + " " + std::visit(AsStringVisitor(), s->rhs);
    }
};

} // namespace

std::string Number::as_string() const {
    return std::to_string(value);
};

std::string Boolean::as_string() const {
    return value ? "true" : "false";
};

std::string String::as_string() const {
    return "'" + value + "'";
};

std::string Identifier::as_string() const {
    return value;
};

// XXX: it would sure be nice not to have duplication here...
std::string UnaryExpression::as_string() const {
    return "-" + std::visit(AsStringVisitor(), rhs);
};

std::string AdditiveExpression::as_string() const {
    // XXX: this is a lie
    return std::visit(AsStringVisitor(), lhs) + " + " + std::visit(AsStringVisitor(), rhs);
};

std::string MultiplicativeExpression::as_string() const {
    // XXX: this is a lie
    return std::visit(AsStringVisitor(), lhs) + " * " + std::visit(AsStringVisitor(), rhs);
};

std::string CodeBlock::as_string() const {
    return std::accumulate(std::begin(expressions), std::end(expressions), std::string{},
                           [](std::string & s, auto const & e) {
                               AsStringVisitor as{};
                               return s.empty() ? std::visit(as, e) : s + ", " + std::visit(as, e);
                           });
}

} // namespace Frontend::AST
