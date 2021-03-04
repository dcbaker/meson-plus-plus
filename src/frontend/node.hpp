// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <cstdint>
#include <string>

namespace Frontend::AST {

class Node {
public:
    virtual ~Node() {};
protected:
    Node() {};
};

class Expression : public Node {};
class Statement : public Node {};

class Number : public Expression {
public:
    Number(const int64_t & number) : value{number} {};
    ~Number() {};

    explicit operator std::string() const {
        return std::to_string(value);
    }
private:
    const int64_t value;
};

class Boolean : public Expression {
public:
    Boolean(const bool & b) : value{b} {};
    ~Boolean() {};

    explicit operator std::string() const {
        return value ? "true" : "false";
    }
private:
    const bool value;
};

class String : public Expression {
public:
    String(const std::string & str) : value{str} {};
    ~String() {};

    explicit operator std::string() const {
        // XXX: should this return '{value}'?
        return value;
    }
private:
    const std::string value;
};

class Identifier : public Expression {
public:
    Identifier(const std::string & str) : value{str} {};
    ~Identifier() {};

    explicit operator std::string() const {
        return value;
    }
private:
    const std::string value;
};

}
