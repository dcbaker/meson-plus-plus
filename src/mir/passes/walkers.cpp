// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include <cassert>

#include "exceptions.hpp"
#include "private.hpp"

namespace MIR::Passes {

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

    auto it = arr->value.begin();
    while (it != arr->value.end()) {
        if (std::holds_alternative<std::unique_ptr<Array>>(*it)) {

            progress |= array_walker(*it, cb);
        } else {
            auto rt = cb(*it);
            if (rt.has_value()) {
                arr->value[it - arr->value.begin()] = std::move(rt.value());
                progress = true;
            }
        }
        ++it;
    }

    return progress;
}

} // namespace MIR::Passes
