// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace Frontend::AST {

class AdditiveExpression;
class Assignment;
class Boolean;
class Identifier;
class MultiplicativeExpression;
class Number;
class String;
class Subscript;
class UnaryExpression;
class Relational;
class FunctionCall;

using ExpressionV =
    std::variant<std::unique_ptr<AdditiveExpression>, std::unique_ptr<Assignment>, std::unique_ptr<Boolean>,
                 std::unique_ptr<Identifier>, std::unique_ptr<MultiplicativeExpression>,
                 std::unique_ptr<UnaryExpression>, std::unique_ptr<Number>, std::unique_ptr<String>,
                 std::unique_ptr<Subscript>, std::unique_ptr<Relational>, std::unique_ptr<FunctionCall>>;

using ExpressionList = std::vector<ExpressionV>;

class Number {
  public:
    Number(const int64_t & number) : value{number} {};
    ~Number(){};

    std::string as_string() const;

    const int64_t value;
};

class Boolean {
  public:
    Boolean(const bool & b) : value{b} {};
    ~Boolean(){};

    std::string as_string() const;

    const bool value;
};

class String {
  public:
    String(const std::string & str) : value{str} {};
    ~String(){};

    std::string as_string() const;

    const std::string value;
};

class Identifier {
  public:
    Identifier(const std::string & str) : value{str} {};
    ~Identifier(){};

    std::string as_string() const;

    const std::string value;
};

class Assignment {
  public:
    Assignment(ExpressionV && l, ExpressionV && r) : lhs{std::move(l)}, rhs{std::move(r)} {
        // TODO: add real error message? or would it be better for this to take an identifier?
        assert(std::holds_alternative<std::unique_ptr<Identifier>>(lhs));
    };
    ~Assignment(){};

    std::string as_string() const;

    const ExpressionV lhs;
    const ExpressionV rhs;
};

class Subscript {
  public:
    Subscript(ExpressionV && l, ExpressionV && r) : lhs{std::move(l)}, rhs{std::move(r)} {};
    ~Subscript(){};

    std::string as_string() const;

    const ExpressionV lhs;
    const ExpressionV rhs;
};

enum class UnaryOp {
    NEG,
};

class UnaryExpression {
  public:
    UnaryExpression(const UnaryOp & o, ExpressionV && r) : op{o}, rhs{std::move(r)} {};
    ~UnaryExpression(){};

    std::string as_string() const;

    const UnaryOp op;
    const ExpressionV rhs;
};

enum class MulOp {
    MUL,
    DIV,
    MOD,
};

class MultiplicativeExpression {
  public:
    MultiplicativeExpression(ExpressionV && l, const MulOp & o, ExpressionV && r)
        : lhs{std::move(l)}, op{o}, rhs{std::move(r)} {};
    ~MultiplicativeExpression(){};

    std::string as_string() const;

    const ExpressionV lhs;
    const MulOp op;
    const ExpressionV rhs;
};

enum class AddOp {
    ADD,
    SUB,
};

class AdditiveExpression {
  public:
    AdditiveExpression(ExpressionV && l, const AddOp & o, ExpressionV && r)
        : lhs{std::move(l)}, op{o}, rhs{std::move(r)} {};
    ~AdditiveExpression(){};

    std::string as_string() const;

    const ExpressionV lhs;
    const AddOp op;
    const ExpressionV rhs;
};

enum class RelationalOp {
    LT,
    LE,
    EQ,
    NE,
    GE,
    GT,
};

static AST::RelationalOp to_relop(const std::string & s) {
    if (s == "<") {
        return AST::RelationalOp::LT;
    } else if (s == "<=") {
        return AST::RelationalOp::LE;
    } else if (s == "==") {
        return AST::RelationalOp::EQ;
    } else if (s == "!=") {
        return AST::RelationalOp::NE;
    } else if (s == ">=") {
        return AST::RelationalOp::GE;
    } else if (s == ">") {
        return AST::RelationalOp::GT;
    }
    assert(false);
}

class Relational {
  public:
    Relational(ExpressionV && l, const std::string & o, ExpressionV && r)
        : lhs{std::move(l)}, op{to_relop(o)}, rhs{std::move(r)} {};
    ~Relational(){};

    std::string as_string() const;

    const ExpressionV lhs;
    const RelationalOp op;
    const ExpressionV rhs;
};

class PositionalArguments {
  public:
    PositionalArguments() : expressions{} {};
    PositionalArguments(ExpressionV && expr) : expressions{} {
        expressions.emplace_back(std::move(expr));
    };
    ~PositionalArguments(){};

    std::string as_string() const;

    ExpressionList expressions;
};

class FunctionCall {
  public:
    FunctionCall(ExpressionV && i, std::unique_ptr<PositionalArguments> && pos) :
      id{std::move(i)}, pos_args{std::move(pos)} {};
    ~FunctionCall(){};

    std::string as_string() const;

    const ExpressionV id;
    const std::unique_ptr<PositionalArguments> pos_args;
};

class CodeBlock {
  public:
    CodeBlock() : expressions{} {};
    CodeBlock(ExpressionV && expr) : expressions{} {
        expressions.emplace_back(std::move(expr));
    };
    ~CodeBlock(){};

    std::string as_string() const;

    // XXX: this should probably be a statement list
    ExpressionList expressions;
};

} // namespace Frontend::AST
