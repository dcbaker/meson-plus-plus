// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

bool number(Object & obj, std::unordered_map<std::string, uint32_t> & data) {
    Variable * var = std::visit([](auto & obj) { return &obj->var; }, obj);
    if (!var) {
        return false;
    }

    // We never revalue a number, otherwise we might end up in a situation where
    // we change the number in the assignment, but then the users point to the
    // wrong thing
    if (var->version > 0) {
        return false;
    }

    if (data.count(var->name) == 0) {
        data[var->name] = 0;
    }

    var->version = ++data[var->name];

    return true;
}

const auto get_var = [](const auto & o) { return o->var; };

// Annotate usages of identifiers, so know if we need to replace them
bool number_uses(const Object & obj, LastSeenTable & table) {
    bool progress = false;

    if (std::holds_alternative<std::unique_ptr<Identifier>>(obj)) {
        const auto & id = std::get<std::unique_ptr<Identifier>>(obj);
        assert(table.find(id->value) != table.end());
        progress |= id->version != table[id->value];
        id->version = table[id->value];
        table[id->var.name] = id->var.version;
    } else if (const Variable & var = std::visit(get_var, obj); var) {
        table[var.name] = var.version;
    }

    return progress;
}

} // namespace

bool value_numbering(BasicBlock * block, std::unordered_map<std::string, uint32_t> & data) {
    return instruction_walker(block, {[&](Object & obj) { return number(obj, data); }});
}

bool usage_numbering(BasicBlock * block, LastSeenTable & table) {
    const auto number = [&](Object & obj) { return number_uses(obj, table); };

    return instruction_walker(block,
                              {
                                  number,
                                  [&](Object & o) { return array_walker(o, number); },
                                  [&](Object & o) { return function_argument_walker(o, number); },
                              });
}

} // namespace MIR::Passes
