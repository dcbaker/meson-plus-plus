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
bool number_uses(const uint32_t & index, const Object & obj, LastSeenTable & tab) {
    bool progress = false;

    auto & table = tab[index];

    if (std::holds_alternative<std::unique_ptr<Identifier>>(obj)) {
        const auto & id = std::get<std::unique_ptr<Identifier>>(obj);
        if (const auto & v = table.find(id->value); v != table.end()) {
            id->version = v->second;
        }
    } else if (std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        const auto & func = std::get<std::shared_ptr<FunctionCall>>(obj);
        if (func->holder &&
            std::holds_alternative<std::unique_ptr<Identifier>>(func->holder.value())) {
            const auto & id = std::get<std::unique_ptr<Identifier>>(func->holder.value());
            if (const auto & v = table.find(id->value); v != table.end()) {
                id->version = v->second;
            }
        }
    }

    if (const Variable & var = std::visit(get_var, obj); var) {
        table[var.name] = var.version;
    }

    return progress;
}

} // namespace

bool value_numbering(BasicBlock & block, std::unordered_map<std::string, uint32_t> & data) {
    return function_walker(block, {[&](Object & obj) { return number(obj, data); }});
}

bool usage_numbering(BasicBlock & block, LastSeenTable & table) {
    const auto number = [&](Object & obj) { return number_uses(block.index, obj, table); };

    table[block.index] = {};
    for (const auto & p : block.parents) {
        table[block.index].merge(table[p->index]);
    }

    return function_walker(block, number);
}

} // namespace MIR::Passes
