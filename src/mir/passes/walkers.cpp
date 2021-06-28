// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include "exceptions.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

bool replace_elements(std::vector<Object> & vec, const ReplacementCallback & cb) {
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

bool instruction_walker(BasicBlock * block, const std::vector<MutationCallback> & fc) {
    return instruction_walker(block, fc, {});
}

bool instruction_walker(BasicBlock * block, const std::vector<ReplacementCallback> & rc) {
    return instruction_walker(block, {}, rc);
}

bool instruction_walker(BasicBlock * block, const std::vector<MutationCallback> & fc,
                        const std::vector<ReplacementCallback> & rc) {
    bool progress = false;

    auto it = block->instructions.begin();
    while (it != block->instructions.end()) {
        for (const auto & cb : rc) {
            auto rt = cb(*it);
            if (rt.has_value()) {
                it = block->instructions.erase(it);
                block->instructions.insert(it, std::move(rt.value()));
                progress |= true;
            }
        }
        for (const auto & cb : fc) {
            progress |= cb(*it);
        }
        ++it;
    }

    return progress;
};

bool array_walker(Object & obj, const ReplacementCallback & cb) {
    bool progress = false;

    if (!std::holds_alternative<std::unique_ptr<Array>>(obj)) {
        return progress;
    }
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

bool function_argument_walker(Object & obj, const ReplacementCallback & cb) {
    bool progress = false;

    if (!std::holds_alternative<std::unique_ptr<FunctionCall>>(obj)) {
        return progress;
    }

    auto & func = std::get<std::unique_ptr<FunctionCall>>(obj);

    if (!func->pos_args.empty()) {
        progress |= replace_elements(func->pos_args, cb);
    }

    // TODO: dictionary lowering

    return progress;
}

} // namespace MIR::Passes
