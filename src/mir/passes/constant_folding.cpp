// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include <cassert>

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

const auto get_var = [](const auto & o) { return o->var; };

std::optional<Object> constant_folding_impl(const Object & obj, ReplacementTable & table) {
    if (std::holds_alternative<std::unique_ptr<Identifier>>(obj)) {
        auto & id = std::get<std::unique_ptr<Identifier>>(obj);
        Variable new_var{id->value, id->version};
        // If this is an assignment populate the table
        if (const Variable & var = std::visit(get_var, obj)) {
            // If we are aliasing an already aliased varaiable replace that immediately
            if (const auto & var2 = table.find(new_var); var2 != table.end()) {
                table[var] = var2->second;
            } else {
                table[var] = new_var;
            }
        } else if (const auto & found = table.find(new_var); found != table.end()) {
            return std::make_unique<Identifier>(found->second.name, found->second.version,
                                                Variable{id->var});
        }
    }
    return std::nullopt;
}

} // namespace

bool constant_folding(BasicBlock * block, ReplacementTable & table) {
    bool progress = false;

    const auto fold = [&](const Object & o) { return constant_folding_impl(o, table); };
    progress |=
        instruction_walker(block,
                           {
                               [&](const Object & o) { return array_walker(o, fold); },
                               [&](const Object & o) { return function_argument_walker(o, fold); },
                           },
                           {fold});

    return progress;
}

} // namespace MIR::Passes
