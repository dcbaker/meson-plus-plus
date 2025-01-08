
// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include "passes.hpp"
#include "private.hpp"

#include <algorithm>

namespace MIR::Passes {

namespace {

/// Recursively call this to flatten nested arrays to function calls
void do_flatten(const ArrayPtr & arr, std::vector<Object> & newarr) {
    for (auto & e : arr->value) {
        if (std::holds_alternative<ArrayPtr>(e)) {
            do_flatten(std::get<ArrayPtr>(e), newarr);
        } else {
            newarr.emplace_back(e);
        }
    }
}

} // namespace

/**
 * Flatten arrays when passed as arguments to functions.
 */
std::optional<Object> flatten(const Object & obj) {
    if (!std::holds_alternative<ArrayPtr>(obj)) {
        return std::nullopt;
    }
    const auto & arr = std::get<ArrayPtr>(obj);

    if (std::none_of(arr->value.begin(), arr->value.end(), [](const Object & inst) {
            return std::holds_alternative<ArrayPtr>(inst);
        })) {
        return std::nullopt;
    }

    std::vector<Object> newarr{};
    do_flatten(arr, newarr);

    return std::make_shared<Array>(std::move(newarr));
}

} // namespace MIR::Passes
