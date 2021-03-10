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

    std::string operator()(const std::unique_ptr<Assignment> & s) {
        return s->as_string();
    }

    std::string operator()(const std::unique_ptr<Subscript> & s) {
        AsStringVisitor as{};
        return std::visit(as, s->lhs) + "[" + std::visit(as, s->rhs) + "]";
    }

    std::string operator()(const std::unique_ptr<Relational> & s) {
        AsStringVisitor as{};

        std::string opstr{};
        switch (s->op) {
            case RelationalOp::LT:
                opstr = "<";
                break;
            case RelationalOp::LE:
                opstr = "<=";
                break;
            case RelationalOp::EQ:
                opstr = "==";
                break;
            case RelationalOp::NE:
                opstr = "!=";
                break;
            case RelationalOp::GE:
                opstr = ">=";
                break;
            case RelationalOp::GT:
                opstr = ">";
                break;
        }

        return std::visit(as, s->lhs) + " " + opstr + " " + std::visit(as, s->rhs);
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

    std::string operator()(const std::unique_ptr<FunctionCall> & s) {
        return s->as_string();
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

std::string Assignment::as_string() const {
    AsStringVisitor as{};
    return std::visit(as, lhs) + " = " + std::visit(as, rhs);
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

std::string Arguments::as_string() const {
    auto pos = std::accumulate(std::begin(positional), std::end(positional), std::string{},
                               [](std::string & s, auto const & e) {
                                   AsStringVisitor as{};
                                   return s.empty() ? std::visit(as, e) : s + ", " + std::visit(as, e);
                               });
    auto kw =
        std::accumulate(std::begin(keyword), std::end(keyword), std::string{}, [](std::string & s, auto const & e) {
            AsStringVisitor as{};
            const auto & [k, a] = e;
            auto v = std::visit(as, k) + " : " + std::visit(as, a);
            if (s.empty()) {
                return v;
            } else {
                return s + ", " + v;
            }
        });

    if (!pos.empty() && !kw.empty()) {
        return pos + ", " + kw;
    } else if (!pos.empty()) {
        return pos;
    } else {
        return kw;
    }
}

std::string FunctionCall::as_string() const {
    auto name = std::visit(AsStringVisitor(), id);
    return name + "(" + args->as_string() + ")";
}

std::string CodeBlock::as_string() const {
    return std::accumulate(std::begin(expressions), std::end(expressions), std::string{},
                           [](std::string & s, auto const & e) {
                               AsStringVisitor as{};
                               return s.empty() ? std::visit(as, e) : s + ", " + std::visit(as, e);
                           });
}

} // namespace Frontend::AST
