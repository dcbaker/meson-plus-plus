// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/**
 * Meson++ High level IR
 *
 * HIR is not much different than our AST, it's still a hierarchical IR, but it
 * is not designed to be losseless as we can do a number of obvious
 * transformations as this level.
 */

#pragma once

#include <cstdint>
#include <list>
#include <memory>
#include <string>
#include <variant>

namespace HIR {

class Boolean;
class String;
class Number;
class FunctionCall;
class Identifier;

using Object =
    std::variant<std::unique_ptr<FunctionCall>, std::unique_ptr<String>, std::unique_ptr<Boolean>,
                 std::unique_ptr<Number>, std::unique_ptr<Identifier>>;

// Can be a method via an optional paramter maybe?
/// A function call object
class FunctionCall {
  public:
    FunctionCall(Object && _name) : name{std::move(_name)} {};

    Object name;
};

class String {
  public:
    String(const std::string & f) : value{f} {};

    const std::string value;
};

class Boolean {
  public:
    Boolean(const bool & f) : value{f} {};

    const bool value;
};

class Number {
  public:
    Number(const int64_t & f) : value{f} {};

    const int64_t value;
};

class Identifier {
  public:
    Identifier(const std::string & s) : value{s} {};

    const std::string value;
};

// TODO: Conditions?
// One way would be to have some kind of phi-like thing,
// Another option would be to have some kind of Conditional, which itself
// contains another IRList (or a pointer to an IRList)

/// List of HIR instructions
using IRList = std::list<Object>;

} // namespace HIR
