
// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

/// Recursively call this to flatten nested arrays to function calls
void do_flatten(const std::shared_ptr<Array> & arr, std::vector<Object> & newarr) {
    for (auto & e : arr->value) {
        if (std::holds_alternative<std::shared_ptr<Array>>(e)) {
            do_flatten(std::get<std::shared_ptr<Array>>(e), newarr);
        } else {
            newarr.emplace_back(std::move(e));
        }
    }
}

/**
 * Flatten arrays when passed as arguments to functions.
 */
std::optional<Object> flatten_cb(const Object & obj) {
    if (!std::holds_alternative<std::shared_ptr<Array>>(obj)) {
        return std::nullopt;
    }

    const auto & arr = std::get<std::shared_ptr<Array>>(obj);

    // If there is nothing to flatten, don't go mutation anything
    bool has_array = false;
    for (const auto & e : arr->value) {
        has_array = std::holds_alternative<std::shared_ptr<Array>>(e);
        if (has_array) {
            break;
        }
    }
    if (!has_array) {
        return std::nullopt;
    }

    std::vector<Object> newarr{};
    do_flatten(arr, newarr);

    return std::make_shared<Array>(std::move(newarr));
}

} // namespace

bool flatten(BasicBlock * block, const State::Persistant & pstate) {
    // TODO: we need to skip this for message, error, and warning
    return function_walker(block, flatten_cb);
}

} // namespace MIR::Passes
