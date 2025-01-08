// SPDX-License-Identifier: Apache-2.0
// Copyright © 2024-2025 Intel Corporation

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

bool combine_add_arguments(std::shared_ptr<CFGNode> block) {
    MIR::AddArgumentsPtr proj = nullptr;
    MIR::AddArgumentsPtr global = nullptr;

    bool progress = false;

    for (auto it = block->block->instructions.begin(); it != block->block->instructions.end();
         ++it) {
        if (std::holds_alternative<MIR::AddArgumentsPtr>(*it)) {
            MIR::AddArgumentsPtr a = std::get<MIR::AddArgumentsPtr>(*it);
            if (a->is_global && global == nullptr) {
                global = a;
                continue;
            } else if (!a->is_global && proj == nullptr) {
                // TODO: project arguments can only be combined if they are from
                // the same sub-project
                proj = a;
                continue;
            }

            MIR::AddArgumentsPtr target = a->is_global ? global : proj;
            // TODO: if this is a project argument, we need to only combine them
            // if they are for the same project
            for (auto && [language, arguments] : a->arguments) {
                if (auto && l = target->arguments.find(language); l != target->arguments.end()) {
                    l->second.insert(l->second.end(), arguments.begin(), arguments.end());
                } else {
                    target->arguments[language] = arguments;
                }
                progress = true;
            }
            // We don't want to go over the same block twice
            it = ++block->block->instructions.erase(it);
        }
    }

    return progress;
}

} // namespace MIR::Passes
