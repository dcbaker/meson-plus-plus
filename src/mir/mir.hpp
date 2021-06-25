// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/**
 * Meson++ Mid level IR
 *
 * This IR is lossy, it doesn't contain all of the information that the AST
 * does, and is designed for running lower passes on, so we can get it closer to
 * the backend IR, removing all function calls and most varibles.
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

namespace MIR {

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

/**
 * Information about an object when it is stored to a variable
 *
 * At the MIR level, assignments are stored to the object, as many
 * objects have creation side effects (creating a Target, for example)
 *
 * The name will be referenced against the symbol table, along with the version
 * which is used by value numbering.
 */
class Variable {
  public:
    Variable() : name{}, version{0} {};

    explicit operator bool() const;

    std::string name;

    /// The version as used by value numbering, 0 means unset
    uint version;
};

// Can be a method via an optional paramter maybe?
/// A function call object
class FunctionCall {
  public:
    FunctionCall(const std::string & _name, std::vector<Object> && _pos,
                 std::unordered_map<std::string, Object> && _kw)
        : name{_name}, pos_args{std::move(_pos)}, kw_args{std::move(_kw)}, var{} {};

    const std::string name;

    /// Ordered container of positional argument objects
    std::vector<Object> pos_args;

    /// Unordered container mapping keyword arguments to their values
    std::unordered_map<std::string, Object> kw_args;

    Variable var;
};

class String {
  public:
    String(const std::string & f) : value{f}, var{} {};

    const std::string value;
    Variable var;
};

class Boolean {
  public:
    Boolean(const bool & f) : value{f}, var{} {};

    const bool value;
    Variable var;
};

class Number {
  public:
    Number(const int64_t & f) : value{f}, var{} {};

    const int64_t value;
    Variable var;
};

class Identifier {
  public:
    Identifier(const std::string & s) : value{s}, var{} {};

    const std::string value;
    Variable var;
};

class Array {
  public:
    Array() : value{}, var{} {};
    Array(std::vector<Object> && a) : value{std::move(a)} {};

    std::vector<Object> value;
    Variable var;
};

class Dict {
  public:
    Dict() : value{}, var{} {};

    // TODO: the key is allowed to be a string or an expression that evaluates
    // to a string, we need to enforce that somewhere.
    std::unordered_map<std::string, Object> value;
    Variable var;
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

} // namespace MIR
