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
#include <map>
#include <optional>
#include <string>
#include <variant>

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

} // namespace HIR
