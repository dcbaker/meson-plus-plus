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

namespace {};

class AdditiveExpression;
class Boolean;
class Identifier;
class MultiplicativeExpression;
class Number;
class String;
class UnaryExpression;

using ExpressionV = std::variant<std::unique_ptr<AdditiveExpression>, std::unique_ptr<Boolean>,
                                 std::unique_ptr<Identifier>, std::unique_ptr<MultiplicativeExpression>,
                                 std::unique_ptr<UnaryExpression>, std::unique_ptr<Number>, std::unique_ptr<String>>;

using ExpressionList = std::vector<ExpressionV>;

class Number {
  public:
    Number(const int64_t & number) : value{number} {};
    ~Number(){};

    explicit operator std::string() const;

    const int64_t value;
};

class Boolean {
  public:
    Boolean(const bool & b) : value{b} {};
    ~Boolean(){};

    explicit operator std::string() const;

    const bool value;
};

class String {
  public:
    String(const std::string & str) : value{str} {};
    ~String(){};

    explicit operator std::string() const;

    const std::string value;
};

class Identifier {
  public:
    Identifier(const std::string & str) : value{str} {};
    ~Identifier(){};

    explicit operator std::string() const;

    const std::string value;
};

class Assignment {
  public:
    Assignment(ExpressionV && l, ExpressionV && r) : lhs{std::move(l)}, rhs{std::move(r)} {
        assert(std::holds_alternative<std::unique_ptr<Identifier>>(lhs));
    };
    ~Assignment(){};

    explicit operator std::string() const;

    const ExpressionV lhs;
    const ExpressionV rhs;
};

class Subscript {
  public:
    Subscript(ExpressionV && l, ExpressionV && r) : lhs{std::move(l)}, rhs{std::move(r)} {};
    ~Subscript(){};

    explicit operator std::string() const;

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

    explicit operator std::string() const;

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

    explicit operator std::string() const;

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

    explicit operator std::string() const;

    const ExpressionV lhs;
    const AddOp op;
    const ExpressionV rhs;
};

class CodeBlock {
  public:
    CodeBlock() : expressions{} {};
    CodeBlock(ExpressionV && expr) : expressions{} {
        expressions.emplace_back(std::move(expr));
    };
    ~CodeBlock(){};

    explicit operator std::string() const;

    // XXX: this should probably be a statement list
    ExpressionList expressions;
};

} // namespace Frontend::AST
