// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "node_visitors.hpp"

namespace Frontend::AST {

std::string as_string::operator()(const std::unique_ptr<String> & s) {
    return std::string{*s};
}

std::string as_string::operator()(const std::unique_ptr<Number> & s) {
    return std::string{*s};
}

std::string as_string::operator()(const std::unique_ptr<Boolean> & s) {
    return std::string{*s};
}

std::string as_string::operator()(const std::unique_ptr<UnaryExpression> & s) {
    // There's currently only unary negation
    return "-" + std::visit(as_string(), s->rhs);
}

std::string as_string::operator()(const std::unique_ptr<AdditiveExpression> & s) {
    std::string o;
    switch (s->op) {
        case AddOp::ADD:
            o = "+";
            break;
        case AddOp::SUB:
            o = "-";
            break;
    }

    return std::visit(as_string(), s->lhs) + " " + o + " " + std::visit(as_string(), s->rhs);
}

std::string as_string::operator()(const std::unique_ptr<MultiplicativeExpression> & s) {
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

    return std::visit(as_string(), s->lhs) + " " + o + " " + std::visit(as_string(), s->rhs);
}

} // namespace Frontend::AST
