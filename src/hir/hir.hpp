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
#include <memory>
#include <string>
#include <variant>
#include <vector>

namespace HIR {

///
template <typename T> class BasicType {
    BasicType(const T & v) : value{v} {};
    virtual ~BasicType(){};

    const T value;
    // TODO: needs location?
};

using Boolean = BasicType<bool>;
using String = BasicType<std::string>;
using Number = BasicType<int64_t>;

/// A variant that holds all the value types
using BasicValues = std::variant<Boolean, String, Number>;

class Variable {
    Variable(std::unique_ptr<BasicValues> && v) : value{std::move(v)} {};
    virtual ~Variable(){};

    std::unique_ptr<BasicValues> value;
};

/// A variable or value
using VariableOrValue = std::variant<BasicValues, Variable>;

/**
 * An executable target
 */
class ExectuableTarget {
    ExectuableTarget(const std::string & n, const std::vector<const std::string> & srcs)
        : name{n}, sources{srcs} {};
    virtual ~ExectuableTarget(){};

    const std::string name;
    const std::vector<const std::string> sources;
};

} // namespace HIR
