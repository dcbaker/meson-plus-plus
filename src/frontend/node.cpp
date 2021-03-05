// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <numeric>

#include "node.hpp"

namespace Frontend::AST {

std::string UnaryExpression::as_string() const {
    std::string o;
    switch (op) {
        case UnaryOp::NEG:
            o = "-";
            break;
    }

    return o + rhs->as_string();
}

std::string MultiplicativeExpression::as_string() const {
    std::string o;
    switch (op) {
        case MulOp::MUL:
            o = "*";
            break;
        case MulOp::DIV:
            o = "/";
            break;
        case MulOp::MOD:
            o = "%";
            break;
    }

    return lhs->as_string() + " " + o + " " + rhs->as_string();
}

std::string AdditiveExpression::as_string() const {
    std::string o;
    switch (op) {
        case AddOp::ADD:
            o = "+";
            break;
        case AddOp::SUB:
            o = "-";
            break;
    }

    return lhs->as_string() + " " + o + " " + rhs->as_string();
}

std::string CodeBlock::as_string() const {
    return std::accumulate(
        std::begin(expressions), std::end(expressions), std::string{},
        [](std::string & s, const std::unique_ptr<Expression> & e) {
            return s.empty() ? e->as_string() : s + ", " + e->as_string();
        });
}

}
