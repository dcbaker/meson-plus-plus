
// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include "passes.hpp"
#include "private.hpp"

#include <algorithm>

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

} // namespace

/**
 * Flatten arrays when passed as arguments to functions.
 */
std::optional<Instruction> flatten(const Instruction & obj) {
    const auto * arr = std::get_if<Array>(obj.obj_ptr.get());
    if (arr == nullptr) {
        return std::nullopt;
    }

    if (std::none_of(arr->value.begin(), arr->value.end(), [](const Instruction & inst) {
            return std::holds_alternative<Array>(inst.object());
        })) {
        return std::nullopt;
    }

    std::vector<Instruction> newarr{};
    do_flatten(*arr, newarr);

    return Array{std::move(newarr)};
}

} // namespace MIR::Passes
