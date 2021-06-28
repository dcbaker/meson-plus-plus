// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

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

} // namespace MIR::Passes
