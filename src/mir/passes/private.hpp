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
using ReplacementCallback = std::function<std::optional<Object>(Object &)>;

/// Callback will return a boolean that progress is mode
using MutationCallback = std::function<bool(Object &)>;

/// Callback to pass to a BlockWalker, probably an instruction_walker
using BlockWalkerCb = std::function<bool(BasicBlock *)>;

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
 * Walks the isntructions of a basic block calling each callback on Function it fins
 *
 * It is the job of each function callback to only act on functions it means to.
 */
bool function_walker(BasicBlock *, const ReplacementCallback &);
bool function_walker(BasicBlock *, const MutationCallback &);

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
bool block_walker(BasicBlock *, const std::vector<BlockWalkerCb> &);

/// Check if all of the arguments have been reduced from ids
bool all_args_reduced(const std::vector<Object> & pos_args,
                      const std::unordered_map<std::string, Object> & kw_args);

template <typename T> std::optional<T> extract_positional_argument(const Object & arg) {
    if (std::holds_alternative<T>(arg)) {
        return std::get<T>(arg);
    }
    // TODO: this ignores invalid arguments
    return std::nullopt;
}

template <typename T>
std::vector<T> extract_variadic_arguments(std::vector<Object>::const_iterator start,
                                          std::vector<Object>::const_iterator end) {
    std::vector<T> nobjs{};
    for (; start != end; start++) {
        if (std::holds_alternative<std::shared_ptr<Array>>(*start)) {
            const auto & arr = *std::get<std::shared_ptr<Array>>(*start);
            const auto & nvals = extract_variadic_arguments<T>(arr.value.begin(), arr.value.end());
            nobjs.insert(nobjs.end(), nvals.begin(), nvals.end());
        } else {
            // TODO: this is going to ignore invalid arghuments
            std::optional<T> arg = extract_positional_argument<T>(*start);
            if (arg) {
                nobjs.emplace_back(arg.value());
            }
        }
    }
    return nobjs;
}

template <typename T>
std::optional<T> extract_keyword_argument(const std::unordered_map<std::string, Object> & kwargs,
                                          const std::string & name) {
    const auto & found = kwargs.find(name);
    if (found == kwargs.end()) {
        return std::nullopt;
    } else if (!std::holds_alternative<T>(found->second)) {
        // XXX: this is just going to ignore invalid arguments…
        return std::nullopt;
    }
    return std::get<T>(found->second);
}

template <typename T>
std::vector<T>
extract_array_keyword_argument(const std::unordered_map<std::string, Object> & kwargs,
                               const std::string & name, const bool & as_list = false) {
    auto found = kwargs.find(name);
    if (found == kwargs.end()) {
        return {};
    } else if (std::holds_alternative<T>(found->second)) {
        return {std::vector<T>{std::get<T>(found->second)}};
    } else if (std::holds_alternative<std::shared_ptr<Array>>(found->second)) {
        std::vector<T> ret{};
        for (const auto & a : std::get<std::shared_ptr<Array>>(found->second)->value) {
            auto arg = extract_positional_argument<T>(a);
            // XXX: also ignores invalid arguments
            if (arg) {
                ret.emplace_back(arg.value());
            }
        }
        return ret;
    }
    // XXX: This ignores invalid arguments
    return {};
}

} // namespace MIR::Passes
