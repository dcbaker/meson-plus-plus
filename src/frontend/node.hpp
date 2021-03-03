/* SPDX-license-identifier: Apache-2.0 */
/* Copyright © 2021 Intel Corporation */

#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace node{

class Statement;
class Expression;
class Assignment;

typedef std::vector<Statement *> StatementList;
typedef std::vector<Expression *> ExpressionList;
typedef std::vector<Assignment *> VariableList;

class Node {
public:
    ~Node() {};
};

class Expression : public Node {};
class Statement : public Node {};

class Integer : public Expression {
private:
    // FIXME: meson doesn't actually have a bounded size for it's numbers
    int64_t value;
public:
    Integer(const int64_t & val) : value{val} {};
    ~Integer() {};
};

class Block : public Expression {
private:
    StatementList statements;
public:
    Block() {};
    ~Block() {};
};

};
