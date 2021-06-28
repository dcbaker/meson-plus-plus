// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

/**
 * Implementation details of the MIR passes
 */

#pragma once

#include "mir.hpp"
#include <functional>
#include <optional>

namespace MIR::Passes {

using Callback = std::function<std::optional<Object>(const Object &)>;
using FilterCallback = std::function<bool(Object &)>;

/**
 * Walks each instruction in a basic block, calling the callback on them
 *
 * If the callback returns a value (not std::nullopt), then it will replace the
 * current instruction with that instruction, otherwise it does nothing.
 *
 * Returns true if any changes were made to the block.
 */
bool instruction_walker(BasicBlock *, const Callback &);

/**
 * Walk each instruction in a basic block, calling the callback on them if they
 * are of the templated type.
 *
 * returns True if any changes were made to the block
 */
template <typename T>
bool instruction_filter_walker(BasicBlock * block, const FilterCallback & cb) {
    bool progress = false;

    for (auto & i : block->instructions) {
        if (std::holds_alternative<T>(i)) {
            progress |= cb(i);
        }
    }

    return progress;
}

/**
 * Walk each instruction in an array, recursively, calling the callbck on them.
 */
bool array_walker(Object &, const Callback &);

} // namespace MIR::Passes
