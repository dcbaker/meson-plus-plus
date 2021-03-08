// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace Frontend::AST {

class Node {
  public:
    virtual ~Node(){};
    virtual std::string as_string() const = 0;

  protected:
    Node(){};
};

class Expression : public Node {
  public:
    virtual ~Expression(){};
    virtual std::string as_string() const = 0;

  protected:
    Expression(){};
};

class Statement : public Node {};

typedef std::vector<std::unique_ptr<Expression>> ExpressionList;

class Number : public Expression {
  public:
    Number(const int64_t & number) : value{number} {};
    ~Number(){};

    virtual std::string as_string() const override {
        return std::to_string(value);
    }

  private:
    const int64_t value;
};

class Boolean : public Expression {
  public:
    Boolean(const bool & b) : value{b} {};
    ~Boolean(){};

    virtual std::string as_string() const override {
        return value ? "true" : "false";
    }

  private:
    const bool value;
};

class String : public Expression {
  public:
    String(const std::string & str) : value{str} {};
    ~String(){};

    virtual std::string as_string() const override {
        return "'" + value + "'";
    }

  private:
    const std::string value;
};

class Identifier : public Expression {
  public:
    Identifier(const std::string & str) : value{str} {};
    ~Identifier(){};

    virtual std::string as_string() const override {
        return value;
    }

  private:
    const std::string value;
};

class Assignment : public Expression {
  public:
    Assignment(std::unique_ptr<Identifier> && l, std::unique_ptr<Expression> && r)
        : lhs{std::move(l)}, rhs{std::move(r)} {};
    ~Assignment(){};

    virtual std::string as_string() const override {
        return lhs->as_string() + " = " + rhs->as_string();
    }

  private:
    const std::unique_ptr<Identifier> lhs;
    const std::unique_ptr<Expression> rhs;
};

class Subscript : public Expression {
  public:
    Subscript(std::unique_ptr<Expression> && l, std::unique_ptr<Expression> && r)
        : lhs{std::move(l)}, rhs{std::move(r)} {};
    ~Subscript(){};

    virtual std::string as_string() const override {
        return lhs->as_string() + "[" + rhs->as_string() + "]";
    }

  private:
    const std::unique_ptr<Expression> lhs;
    const std::unique_ptr<Expression> rhs;
};

enum class UnaryOp {
    NEG,
};

class UnaryExpression : public Expression {
  public:
    UnaryExpression(const UnaryOp & o, std::unique_ptr<Expression> && r) : op{o}, rhs{std::move(r)} {};
    ~UnaryExpression(){};

    virtual std::string as_string() const override;

  private:
    const UnaryOp op;
    const std::unique_ptr<Expression> rhs;
};

enum class MulOp {
    MUL,
    DIV,
    MOD,
};

class MultiplicativeExpression : public Expression {
  public:
    MultiplicativeExpression(std::unique_ptr<Expression> && l, const MulOp & o, std::unique_ptr<Expression> && r)
        : lhs{std::move(l)}, op{o}, rhs{std::move(r)} {};
    ~MultiplicativeExpression(){};

    virtual std::string as_string() const override;

  private:
    const std::unique_ptr<Expression> lhs;
    const MulOp op;
    const std::unique_ptr<Expression> rhs;
};

enum class AddOp {
    ADD,
    SUB,
};

class AdditiveExpression : public Expression {
  public:
    AdditiveExpression(std::unique_ptr<Expression> && l, const AddOp & o, std::unique_ptr<Expression> && r)
        : lhs{std::move(l)}, op{o}, rhs{std::move(r)} {};
    ~AdditiveExpression(){};

    virtual std::string as_string() const override;

  private:
    const std::unique_ptr<Expression> lhs;
    const AddOp op;
    const std::unique_ptr<Expression> rhs;
};

class CodeBlock : public Expression {
  public:
    CodeBlock() : expressions{} {};
    CodeBlock(std::unique_ptr<Expression> & expr) : expressions{} {
        expressions.emplace_back(std::move(expr));
    };
    ~CodeBlock(){};

    virtual std::string as_string() const override;

    // XXX: this should probably be a statement list
    ExpressionList expressions;
};

} // namespace Frontend::AST
