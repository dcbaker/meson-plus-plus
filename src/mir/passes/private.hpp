// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

/**
 * Implementation details of the MIR passes
 */

#pragma once

#include "mir.hpp"
#include <functional>

namespace MIR::Passes {

/// Callback will return a an optional Object, when it does the original object is replaced
using ReplacementCallback = std::function<std::optional<Object>(Object &)>;

/// Callback will return a boolean that progress is mode
using MutationCallback = std::function<bool(Object &)>;

/// Callback to pass to a BlockWalker, probably an instruction_walker
using BlockWalkerCb = std::function<bool(BasicBlock &)>;

/**
 * Walks each instruction in a basic block, calling each callback on each instruction
 *
 * Returns true if any changes were made to the block.
 */
bool instruction_walker(BasicBlock &, const std::vector<MutationCallback> &,
                        const std::vector<ReplacementCallback> &);
bool instruction_walker(BasicBlock &, const std::vector<MutationCallback> &);
bool instruction_walker(BasicBlock &, const std::vector<ReplacementCallback> &);

/**
 * Walks the isntructions of a basic block calling each callback on Function it fins
 *
 * It is the job of each function callback to only act on functions it means to.
 */
bool function_walker(BasicBlock &, const ReplacementCallback &);
bool function_walker(BasicBlock &, const MutationCallback &);

/**
 * Walk each instruction in an array, recursively, calling the callbck on them.
 */
bool array_walker(const Object &, const ReplacementCallback &);
bool array_walker(Object &, const MutationCallback &);

/**
 * Walk over the arguments (positional and keyword) of a function
 *
 * This will replace the arguments if they are loweed by the callback
 */
bool function_argument_walker(const Object &, const ReplacementCallback &);
bool function_argument_walker(Object &, const MutationCallback &);

/**
 * Walker over all basic blocks starting with the provided one, applying the given callbacks
 */
bool block_walker(BasicBlock &, const std::vector<BlockWalkerCb> &);

/// Check if all of the arguments have been reduced from ids
bool all_args_reduced(const std::vector<Object> & pos_args,
                      const std::unordered_map<std::string, Object> & kw_args);

} // namespace MIR::Passes
