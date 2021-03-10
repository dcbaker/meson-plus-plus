// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <numeric>

#include "node.hpp"
#include "node_visitors.hpp"

namespace Frontend::AST {

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
