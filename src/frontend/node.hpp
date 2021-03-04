// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <memory>

namespace Frontend::AST {

class Node {
public:
    virtual ~Node() {};
    virtual std::string as_string() const = 0;
protected:
    Node() {};
};

class Expression : public Node {
public:
    Expression() {};
    ~Expression() {};

    virtual std::string as_string() const {
        return "Base Expression";
    }
};

class Statement : public Node {};

typedef std::vector<std::unique_ptr<Expression>> ExpressionList;

class Number : public Expression {
public:
    Number(const int64_t & number) : value{number} {};
    ~Number() {};

    virtual std::string as_string() const override {
        return std::to_string(value);
    }
private:
    const int64_t value;
};

class Boolean : public Expression {
public:
    Boolean(const bool & b) : value{b} {};
    ~Boolean() {};

    virtual std::string as_string() const override {
        return value ? "true" : "false";
    }
private:
    const bool value;
};

class String : public Expression {
public:
    String(const std::string & str) : value{str} {};
    ~String() {};

    virtual std::string as_string() const override {
        return "'" + value + "'";
    }
private:
    const std::string value;
};

class Identifier : public Expression {
public:
    Identifier(const std::string & str) : value{str} {};
    ~Identifier() {};

    virtual std::string as_string() const override {
        return value;
    }
private:
    const std::string value;
};

class Assignment : public Expression {
public:
    Assignment(std::unique_ptr<Identifier> && l, std::unique_ptr<Expression> && r) :
        lhs{std::move(l)}, rhs{std::move(r)} {};
    ~Assignment() {};

    virtual std::string as_string() const override {
        return lhs->as_string() + " = " + rhs->as_string();
    }
private:
    const std::unique_ptr<Identifier> lhs;
    const std::unique_ptr<Expression> rhs;
};

class CodeBlock : public Expression {
public:
    CodeBlock() : expressions{} {} ;
    CodeBlock(std::unique_ptr<Expression> & expr) : expressions{} {
        expressions.push_back(std::move(expr));
    };
    ~CodeBlock() {};

    virtual std::string as_string() const override;

    // XXX: this should probably be a statement list
    ExpressionList expressions;
};

}
