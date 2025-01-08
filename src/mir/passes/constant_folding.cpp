// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include <cassert>

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

std::optional<Object> ConstantFolding::impl(const Object & obj) {
    if (std::holds_alternative<IdentifierPtr>(obj)) {
        const auto & id = std::get<IdentifierPtr>(obj);
        const Variable new_var{id->value, id->version};

        if (const auto & found = data.find(new_var); found != data.end()) {
            /* If the id is already in the table we want to map the alias
             * directly such as:
             *
             *     x₁ = 7
             *     y₁ = x₁
             *     z₁ = y₁
             *
             * In this case we know that z₁ == x₁, and we want to just go ahead
             * and optimize that.
             */

            const Variable & var = std::visit(VariableGetter{}, obj);

            if (var) {
                data[var] = Variable{found->second.name, found->second.gvn};
            }
            auto i = std::make_shared<Identifier>(found->second.name, found->second.gvn);
            i->var = var;
            return i;
        }
        if (auto v = std::visit(VariableGetter{}, obj)) {
            data[v] = new_var;
        }
    }
    return std::nullopt;
}

bool ConstantFolding::operator()(std::shared_ptr<CFGNode> block) {
    return instruction_walker(*block, {[this](const Object & i) { return this->impl(i); }});
};

} // namespace MIR::Passes
