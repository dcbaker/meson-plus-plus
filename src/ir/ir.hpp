// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <cstdint>
#include <list>
#include <optional>
#include <string>
#include <variant>

namespace IR {

/**
 * A Meson string.
 */
class String {
  public:
    String(){};
    virtual ~String(){};
};

using IRVariant = std::variant<String>;

class Instruction {
  public:
    Instruction(IRVariant & v) : var{v}, name{std::nullopt}, version{0} {};
    Instruction(IRVariant & v, const std::string & n, const uint64_t & ver) : var{v}, name{n}, version{ver} {};
    virtual ~Instruction(){};

    /// The actual IR element being held
    IRVariant var;

    /**
     * The name of the variable holding the value
     *
     * This is optional, as many Meson functions do not have values, just
     * side-effects. For example, calling `execurable(...)` will cause
     * something to happen in the backend, even without the assignment.
     */
    const std::optional<const std::string> name;

    /// The version of the variable for value-numbering
    const uint64_t version;
};

/// List of IR Instructions
using IRList = std::list<Instruction>;

/**
 * Basic cdoe block
 *
 * Holds continguous instructions that are to be executed in order.
 */
class BasicBlock {
  public:
    BasicBlock() : instruction{} {};
    virtual ~BasicBlock(){};

    IRList instruction;
};

using BlockList = std::list<BasicBlock>;

} // namespace IR
