// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include <cassert>

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

std::optional<Object> constant_folding_impl(const Object & obj, ReplacementTable & table) {
    if (std::holds_alternative<std::unique_ptr<Identifier>>(obj)) {
        auto & id = std::get<std::unique_ptr<Identifier>>(obj);
        const Variable new_var{id->value, id->version};

        if (const auto & found = table.find(new_var); found != table.end()) {
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

            if (id->var) {
                table[id->var] = Variable{found->second.name, found->second.version};
            }
            return std::make_unique<Identifier>(found->second.name, found->second.version,
                                                Variable{id->var});
        }
        if (id->var) {
            table[id->var] = new_var;
        }
    }
    return std::nullopt;
}

} // namespace

bool constant_folding(BasicBlock & block, ReplacementTable & table) {
    return function_walker(block,
                           {[&](const Object & o) { return constant_folding_impl(o, table); }});
}

} // namespace MIR::Passes
