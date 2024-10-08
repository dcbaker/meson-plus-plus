// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <cassert>

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

std::optional<Instruction> ConstantFolding::impl(const Instruction & obj) {
    auto id = std::get_if<Identifier>(obj.obj_ptr.get());
    if (id != nullptr) {
        const Variable new_var{id->value, id->version};

        if (const auto & found = data.find(new_var); found != data.end()) {
            /* If the id is already in the table we want to map the alias
             * directly such as:
             *
             *     x₁ = 7
             *     y₁ = x₁
             *     z₁ = y₁
             *
             * In this caswe we konw that z₁ == x₁, and we want to just go ahead
             * and optimize that.
             */

            if (obj.var) {
                data[obj.var] = Variable{found->second.name, found->second.version};
            }
            return Instruction{Identifier{found->second.name, found->second.version}, obj.var};
        }
        if (obj.var) {
            data[obj.var] = new_var;
        }
    }
    return std::nullopt;
}

bool ConstantFolding::operator()(BasicBlock & block) {
    return function_walker(block, [this](Instruction & i) { return this->impl(i); });
};

} // namespace MIR::Passes
