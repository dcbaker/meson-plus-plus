
// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

/// Recursively call this to flatten nested arrays to function calls
void do_flatten(const Array & arr, std::vector<Instruction> & newarr) {
    for (auto & e : arr.value) {
        if (std::holds_alternative<Array>(*e.obj_ptr)) {
            do_flatten(std::get<Array>(*e.obj_ptr), newarr);
        } else {
            newarr.emplace_back(e);
        }
    }
}

/**
 * Flatten arrays when passed as arguments to functions.
 */
std::optional<Instruction> flatten_cb(const Instruction & obj) {
    const auto * arr = std::get_if<Array>(obj.obj_ptr.get());
    if (arr == nullptr) {
        return std::nullopt;
    }

    // If there is nothing to flatten, don't go mutation anything
    bool has_array = false;
    for (const auto & e : arr->value) {
        has_array = std::holds_alternative<Array>(*e.obj_ptr);
        if (has_array) {
            break;
        }
    }
    if (!has_array) {
        return std::nullopt;
    }

    std::vector<Instruction> newarr{};
    do_flatten(*arr, newarr);

    return Array{std::move(newarr)};
}

} // namespace

bool flatten(std::shared_ptr<BasicBlock> block, const State::Persistant & pstate) {
    // TODO: we need to skip this for message, error, and warning
    return function_walker(*block, flatten_cb);
}

} // namespace MIR::Passes
