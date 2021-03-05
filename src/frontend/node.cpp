// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <numeric>

#include "node.hpp"

namespace Frontend::AST {

std::string UnaryExpression::as_string() const {
    std::string o;
    switch (op) {
        case UnaryOpEnum::NOT:
            o = "not";
            break;
        case UnaryOpEnum::PLUS:
            o = "+";
            break;
        case UnaryOpEnum::MINUS:
            o = "+";
            break;
    }

    return o + " " + rhs->as_string();
}

std::string MultiplicativeExpression::as_string() const {
    std::string o;
    switch (op) {
        case MulOpEnum::MUL:
            o = "*";
            break;
        case MulOpEnum::DIV:
            o = "/";
            break;
        case MulOpEnum::MOD:
            o = "%";
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
