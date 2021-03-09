// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "node.hpp"
#include "node_visitors.hpp"

namespace Frontend::AST {

Number::operator std::string() const {
    return std::to_string(value);
};

Boolean::operator std::string() const {
    return value ? "true" : "false";
};

String::operator std::string() const {
    return "'" + value + "'";
};

// XXX: it would sure be nice not to have duplication here...
UnaryExpression::operator std::string() const {
    return "-" + std::visit(as_string(), rhs);
};

AdditiveExpression::operator std::string() const {
    // XXX: this is a lie
    return std::visit(as_string(), lhs) + " + " + std::visit(as_string(), rhs);
};

MultiplicativeExpression::operator std::string() const {
    // XXX: this is a lie
    return std::visit(as_string(), lhs) + " * " + std::visit(as_string(), rhs);
};

} // namespace Frontend::AST
