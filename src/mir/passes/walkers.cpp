// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include <cassert>

#include "exceptions.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

bool replace_elements(std::vector<Object> & vec, const Callback & cb) {
    bool progress = false;
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        auto rt = cb(*it);
        if (rt.has_value()) {
            vec[it - vec.begin()] = std::move(rt.value());
            progress |= true;
        }
    }
    return progress;
}

} // namespace

bool instruction_walker(BasicBlock * block, const Callback & cb) {
    bool progress = false;

    auto it = block->instructions.begin();
    while (it != block->instructions.end()) {
        auto & i = *it;
        auto rt = cb(i);
        if (rt.has_value()) {
            it = block->instructions.erase(it);
            block->instructions.insert(it, std::move(rt.value()));
            progress = true;
        }
        ++it;
    }

    return progress;
};

bool array_walker(Object & obj, const Callback & cb) {
    bool progress = false;

    assert(std::holds_alternative<std::unique_ptr<Array>>(obj));
    auto & arr = std::get<std::unique_ptr<Array>>(obj);

    for (auto it = arr->value.begin(); it != arr->value.end(); ++it) {
        if (std::holds_alternative<std::unique_ptr<Array>>(*it)) {
            progress |= array_walker(*it, cb);
        } else {
            auto rt = cb(*it);
            if (rt.has_value()) {
                arr->value[it - arr->value.begin()] = std::move(rt.value());
                progress |= true;
            }
        }
    }

    return progress;
}

bool function_argument_walker(Object & obj, const Callback & cb) {
    bool progress = false;

    assert(std::holds_alternative<std::unique_ptr<FunctionCall>>(obj));
    auto & func = std::get<std::unique_ptr<FunctionCall>>(obj);

    if (!func->pos_args.empty()) {
        progress |= replace_elements(func->pos_args, cb);
    }

    // TODO: dictionary lowering

    return progress;
}

} // namespace MIR::Passes
