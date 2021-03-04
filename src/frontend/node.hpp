// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <cstdint>
#include <string>

namespace Frontend::AST {

class Node {
public:
    Node() {};
    ~Node() {};
};

class Expression : public Node {};
class Statement : public Node {};

class Number : public Node {
public:
    Number(const int64_t & number) : value{number} {};
    ~Number() {};

    explicit operator std::string() const {
        return std::to_string(value);
    }
private:
    int64_t value;
};

class Boolean : public Node {
public:
    Boolean(const bool & b) : value{b} {};
    ~Boolean() {};

    explicit operator std::string() const {
        return value ? "true" : "false";
    }
private:
    bool value;
};

}
