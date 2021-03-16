// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <tuple>
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
class MethodCall;
class Array;

using ExpressionV =
    std::variant<std::unique_ptr<AdditiveExpression>, std::unique_ptr<Assignment>, std::unique_ptr<Boolean>,
                 std::unique_ptr<Identifier>, std::unique_ptr<MultiplicativeExpression>,
                 std::unique_ptr<UnaryExpression>, std::unique_ptr<Number>, std::unique_ptr<String>,
                 std::unique_ptr<Subscript>, std::unique_ptr<Relational>, std::unique_ptr<FunctionCall>,
                 std::unique_ptr<MethodCall>, std::unique_ptr<Array>>;

using ExpressionList = std::vector<ExpressionV>;

class Number {
  public:
    Number(const int64_t & number) : value{number} {};
    Number(const Number &) = delete;
    ~Number(){};

    std::string as_string() const;

    const int64_t value;
};

class Boolean {
  public:
    Boolean(const bool & b) : value{b} {};
    Boolean(const Boolean &) = delete;
    ~Boolean(){};

    std::string as_string() const;

    const bool value;
};

class String {
  public:
    String(const std::string & str) : value{str} {};
    String(const String &) = delete;
    ~String(){};

    std::string as_string() const;

    const std::string value;
};

class Identifier {
  public:
    Identifier(const std::string & str) : value{str} {};
    Identifier(const Identifier &) = delete;
    ~Identifier(){};

    std::string as_string() const;

    const std::string value;
};

class Assignment {
  public:
    Assignment(ExpressionV && l, ExpressionV && r) : lhs{std::move(l)}, rhs{std::move(r)} {
        // TODO: add real error message? or would it be better for this to take an identifier?
        // Or is this really not a parsing issue, but a semantics issue?
        assert(std::holds_alternative<std::unique_ptr<Identifier>>(lhs));
    };
    Assignment(const Assignment &) = delete;
    ~Assignment(){};

    std::string as_string() const;

    const ExpressionV lhs;
    const ExpressionV rhs;
};

class Subscript {
  public:
    Subscript(ExpressionV && l, ExpressionV && r) : lhs{std::move(l)}, rhs{std::move(r)} {};
    Subscript(const Subscript &) = delete;
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
    UnaryExpression(const UnaryExpression &) = delete;
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
    MultiplicativeExpression(const MultiplicativeExpression &) = delete;
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
    AdditiveExpression(const AdditiveExpression &) = delete;
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
    AND,
    OR,
    IN,
    NOT_IN,
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
    } else if (s == "and") {
        return AST::RelationalOp::AND;
    } else if (s == "or") {
        return AST::RelationalOp::OR;
    } else if (s == "in") {
        return AST::RelationalOp::IN;
    } else if (s == "not in") {
        return AST::RelationalOp::NOT_IN;
    }
    assert(false);
}

class Relational {
  public:
    Relational(ExpressionV && l, const std::string & o, ExpressionV && r)
        : lhs{std::move(l)}, op{to_relop(o)}, rhs{std::move(r)} {};
    Relational(const Relational &) = delete;
    ~Relational(){};

    std::string as_string() const;

    const ExpressionV lhs;
    const RelationalOp op;
    const ExpressionV rhs;
};

// XXX: this isn't really true, it's really an identifier : expressionv
using KeywordPair = std::tuple<ExpressionV, ExpressionV>;
using KeywordList = std::vector<KeywordPair>;

class Arguments {
  public:
    Arguments() : positional{} {};
    Arguments(ExpressionList && v) : positional{std::move(v)}, keyword{} {};
    Arguments(KeywordList && k) : positional{}, keyword{std::move(k)} {};
    Arguments(ExpressionList && v, KeywordList && k) : positional{std::move(v)}, keyword{std::move(k)} {};
    Arguments(const Arguments &) = delete;
    ~Arguments(){};

    std::string as_string() const;

    ExpressionList positional;
    KeywordList keyword;
};

class FunctionCall {
  public:
    FunctionCall(ExpressionV && i, std::unique_ptr<Arguments> && a) : id{std::move(i)}, args{std::move(a)} {};
    FunctionCall(const FunctionCall &) = delete;
    ~FunctionCall(){};

    std::string as_string() const;

    const ExpressionV id;
    const std::unique_ptr<Arguments> args;
};

class MethodCall {
  public:
    MethodCall(ExpressionV && o, ExpressionV && i, std::unique_ptr<Arguments> && a)
        : object{std::move(o)}, id{std::move(i)}, args{std::move(a)} {};
    MethodCall(const MethodCall &) = delete;
    ~MethodCall(){};

    std::string as_string() const;

    const ExpressionV object;
    const ExpressionV id;
    const std::unique_ptr<Arguments> args;
};

class Array {
  public:
    Array() : elements{} {};
    Array(ExpressionList && e) : elements{std::move(e)} {};
    Array(const Array &) = delete;
    ~Array(){};

    std::string as_string() const;

    const ExpressionList elements;
};

class Statement {
  public:
    Statement(ExpressionV && e) : expr{std::move(e)} {};
    Statement(const Statement &) = delete;
    ~Statement(){};

    std::string as_string() const;

    const ExpressionV expr;
};

using StatementV = std::variant<std::unique_ptr<Statement>>;

class CodeBlock {
  public:
    CodeBlock() : statements{} {};
    CodeBlock(StatementV && stmt) : statements{} {
        statements.emplace_back(std::move(stmt));
    };
    CodeBlock(const CodeBlock &) = delete;
    ~CodeBlock(){};

    std::string as_string() const;

    // XXX: this should probably be a statement list
    std::vector<StatementV> statements;
};

} // namespace Frontend::AST
