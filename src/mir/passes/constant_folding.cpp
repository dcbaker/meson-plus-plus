// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include <cassert>

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

std::optional<Instruction> constant_folding_impl(const Instruction & obj,
                                                 ReplacementTable & table) {
    auto id = std::get_if<Identifier>(obj.obj_ptr.get());
    if (id != nullptr) {
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

            if (obj.var) {
                table[obj.var] = Variable{found->second.name, found->second.version};
            }
            return Instruction{Identifier{found->second.name, found->second.version}, obj.var};
        }
        if (obj.var) {
            table[obj.var] = new_var;
        }
    }
    return std::nullopt;
}

} // namespace

bool constant_folding(BasicBlock & block, ReplacementTable & table) {
    return function_walker(
        block, {[&](const Instruction & o) { return constant_folding_impl(o, table); }});
}

} // namespace MIR::Passes
