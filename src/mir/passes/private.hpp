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

/// Callback will return a an optional Object, when it does the original object is replaced
using ReplacementCallback = std::function<std::optional<Object>(const Object &)>;

/// Callback will return a boolean that progress is mode
using MutationCallback = std::function<bool(Object &)>;

/**
 * Walks each instruction in a basic block, calling each callback on each instruction
 *
 * Returns true if any changes were made to the block.
 */
bool instruction_walker(BasicBlock *, const std::vector<MutationCallback> &,
                        const std::vector<ReplacementCallback> &);
bool instruction_walker(BasicBlock *, const std::vector<MutationCallback> &);
bool instruction_walker(BasicBlock *, const std::vector<ReplacementCallback> &);

/**
 * Walk each instruction in an array, recursively, calling the callbck on them.
 */
bool array_walker(Object &, const ReplacementCallback &);

/**
 * Walk over the arguments (positional and keyword) of a function
 *
 * This will replace the arguments if they are loweed by the callback
 */
bool function_argument_walker(Object &, const ReplacementCallback &);

} // namespace MIR::Passes
