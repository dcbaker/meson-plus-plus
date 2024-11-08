// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

/**
 * Implementation details of the MIR passes
 */

#pragma once

#include "mir.hpp"
#include <functional>

namespace MIR::Passes {

/// Callback will return a an optional Instruction, when it does the original object is replaced
using ReplacementCallback = std::function<std::optional<Instruction>(const Instruction &)>;

/// Callback will return a boolean that progress is mode
using MutationCallback = std::function<bool(Instruction &)>;

/// Callback to pass to a BlockWalker, probably an instruction_walker
using BlockWalkerCb = std::function<bool(std::shared_ptr<CFGNode>)>;

/**
 * Walks each instruction in a basic block, calling each callback on each instruction
 *
 * Returns true if any changes were made to the block.
 */
bool instruction_walker(CFGNode &, const std::vector<MutationCallback> &,
                        const std::vector<ReplacementCallback> &);
bool instruction_walker(CFGNode &, const std::vector<MutationCallback> &);
bool instruction_walker(CFGNode &, const std::vector<ReplacementCallback> &);

/**
 * Walker over all basic blocks starting with the provided one, applying the given callbacks
 */
bool graph_walker(std::shared_ptr<CFGNode>, const std::vector<BlockWalkerCb> &);

/// Check if all of the arguments have been reduced from ids
bool all_args_reduced(const std::vector<Instruction> & pos_args,
                      const std::unordered_map<std::string, Instruction> & kw_args);

} // namespace MIR::Passes
