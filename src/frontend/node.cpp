// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <numeric>

#include "node.hpp"
#include "node_visitors.hpp"

namespace Frontend::AST {

namespace {

struct ExprStringVisitor {
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

    std::string operator()(const std::unique_ptr<Subscript> & s) {
        ExprStringVisitor as{};
        return std::visit(as, s->lhs) + "[" + std::visit(as, s->rhs) + "]";
    }

    std::string operator()(const std::unique_ptr<Relational> & s) {
        ExprStringVisitor as{};

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
            case RelationalOp::AND:
                opstr = "and";
                break;
            case RelationalOp::OR:
                opstr = "or";
                break;
            case RelationalOp::IN:
                opstr = "in";
                break;
            case RelationalOp::NOT_IN:
                opstr = "not in";
                break;
        }

        return std::visit(as, s->lhs) + " " + opstr + " " + std::visit(as, s->rhs);
    }

    std::string operator()(const std::unique_ptr<UnaryExpression> & s) {
        // There's currently only unary negation
        return "-" + std::visit(ExprStringVisitor(), s->rhs);
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

        return std::visit(ExprStringVisitor(), s->lhs) + " " + o + " " + std::visit(ExprStringVisitor(), s->rhs);
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

        return std::visit(ExprStringVisitor(), s->lhs) + " " + o + " " + std::visit(ExprStringVisitor(), s->rhs);
    }

    std::string operator()(const std::unique_ptr<FunctionCall> & s) {
        return s->as_string();
    }

    std::string operator()(const std::unique_ptr<GetAttribute> & s) {
        return s->as_string();
    }

    std::string operator()(const std::unique_ptr<Array> & s) {
        return s->as_string();
    }

    std::string operator()(const std::unique_ptr<Dict> & s) {
        return s->as_string();
    }
};

struct StmtStringVisitor {
    std::string operator()(const std::unique_ptr<Statement> & s) {
        return s->as_string();
    }

    std::string operator()(const std::unique_ptr<IfStatement> & s) {
        return s->as_string();
    }

    std::string operator()(const std::unique_ptr<Assignment> & s) {
        return s->as_string();
    }
};

/**
 * Convert an A list of ExperssionV elements into a comma separated string
 */
std::string stringlistify(const ExpressionList & expressions) {
    return std::accumulate(std::begin(expressions), std::end(expressions), std::string{},
                           [](std::string & s, auto const & e) {
                               ExprStringVisitor as{};
                               return s.empty() ? std::visit(as, e) : s + ", " + std::visit(as, e);
                           });
}

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
    ExprStringVisitor as{};
    return std::visit(as, lhs) + " = " + std::visit(as, rhs);
};

// XXX: it would sure be nice not to have duplication here...
std::string UnaryExpression::as_string() const {
    return "-" + std::visit(ExprStringVisitor(), rhs);
};

std::string AdditiveExpression::as_string() const {
    // XXX: this is a lie
    return std::visit(ExprStringVisitor(), lhs) + " + " + std::visit(ExprStringVisitor(), rhs);
};

std::string MultiplicativeExpression::as_string() const {
    // XXX: this is a lie
    return std::visit(ExprStringVisitor(), lhs) + " * " + std::visit(ExprStringVisitor(), rhs);
};

std::string Arguments::as_string() const {
    auto pos = stringlistify(positional);
    auto kw =
        std::accumulate(std::begin(keyword), std::end(keyword), std::string{}, [](std::string & s, auto const & e) {
            ExprStringVisitor as{};
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
    auto name = std::visit(ExprStringVisitor(), id);
    return name + "(" + args->as_string() + ")";
}

std::string GetAttribute::as_string() const {
    ExprStringVisitor as{};
    auto obj = std::visit(as, object);
    auto name = std::visit(as, id);
    return obj + "." + name;
}

std::string Array::as_string() const {
    return "[" + stringlistify(elements) + "]";
}

std::string Statement::as_string() const {
    return std::visit(ExprStringVisitor{}, expr);
}

Dict::Dict(KeywordList && l) {
    for (auto & e : l) {
        auto && [k, v] = e;
        elements[std::move(k)] = std::move(v);
    }
};

std::string Dict::as_string() const {
    auto es =
        std::accumulate(std::begin(elements), std::end(elements), std::string{}, [](std::string & s, auto const & e) {
            ExprStringVisitor as{};
            const auto & [k, a] = e;
            auto v = std::visit(as, k) + " : " + std::visit(as, a);
            return s.empty() ? v : s + ", " + v;
        });
    return "{" + es + "}";
}

std::string CodeBlock::as_string() const {
    return std::accumulate(std::begin(statements), std::end(statements), std::string{},
                           [](std::string & s, auto const & e) {
                               StmtStringVisitor as{};
                               return s.empty() ? std::visit(as, e) : s + ", " + std::visit(as, e);
                           });
}

std::string IfStatement::as_string() const {
    return "TODO";
}

} // namespace Frontend::AST
