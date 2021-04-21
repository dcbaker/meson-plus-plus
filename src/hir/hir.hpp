// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <cstdint>
#include <list>
#include <map>
#include <optional>
#include <string>
#include <variant>

namespace HIR {

/**
 * A Generic holder type.
 *
 * This is used to hold assignments of a specific type and it's assignment
 * information
 */
template <typename T> class Holder {
  public:
    Holder(const T & v, const T & n) : value{v}, variable_name{n} {};
    Holder(const T & v) : value{v}, variable_name{std::nullopt} {};
    virtual ~Holder(){};

    const T value;
    const std::optional<T> variable_name;
    std::optional<std::uint16_t> value_number = std::nullopt;
};

using StringHolder = Holder<std::string>;
using NumberHolder = Holder<std::int64_t>;

using Holders = std::variant<StringHolder, NumberHolder>;

/// List of IR Instructions
using HolderList = std::list<Holders>;

class BasicBlock;

class Phi {
    Phi(){};
    virtual ~Phi(){};

    std::map<HolderList, BasicBlock *> targets;
};

/**
 * Basic cdoe block
 *
 * Holds continguous instructions that are to be executed in order, and
 * possibly a phi node.
 *
 * For our purposes a function call can appear inside a basic block because we
 * lack user defined functions, so we can think of a function or method call
 * not as a jump to a different basic block, but as value itself, in other
 * words, these are equivalent:
 * ```meson
 * a = ['gcc']
 * b = cc.get_compiler('c')
 * ```
 */
class BasicBlock {
  public:
    BasicBlock() : instructions{}, phi{std::nullopt} {};
    virtual ~BasicBlock(){};

    HolderList instructions;
    std::optional<Phi> phi;
};

} // namespace HIR
