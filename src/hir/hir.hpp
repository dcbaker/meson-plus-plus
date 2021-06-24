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
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

namespace HIR {

class Array;
class Boolean;
class Dict;
class FunctionCall;
class Identifier;
class Number;
class String;

using Object =
    std::variant<std::unique_ptr<FunctionCall>, std::unique_ptr<String>, std::unique_ptr<Boolean>,
                 std::unique_ptr<Number>, std::unique_ptr<Identifier>, std::unique_ptr<Array>,
                 std::unique_ptr<Dict>>;

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

class Array {
  public:
    Array() : value{} {};
    Array(std::vector<Object> && a) : value{std::move(a)} {};

    std::vector<Object> value;
};

class Dict {
  public:
    Dict() : value{} {};

    // TODO: the key is allowed to be a string or an expression that evaluates
    // to a string, we need to enforce that somewhere.
    std::unordered_map<std::string, Object> value;
};

class IRList;

/**
 * A sort of phi-like thing that holds a condition and two branches
 */
class Condition {
  public:
    Condition(Object && o)
        : condition{std::move(o)}, if_true{std::make_unique<IRList>()},
          // We could save a bit of memory here by not initializing if_false, but
          // that means more manual tracking for a tiny savings…
          if_false{std::make_unique<IRList>()} {};

    Object condition;
    std::unique_ptr<IRList> if_true;
    std::unique_ptr<IRList> if_false;
};

/**
 * Holds a list of instructions, and optionally a condition
 *
 */
class IRList {
  public:
    IRList() : instructions{}, condition{std::nullopt} {};

    std::list<Object> instructions;
    std::optional<Condition> condition;
};

} // namespace HIR
