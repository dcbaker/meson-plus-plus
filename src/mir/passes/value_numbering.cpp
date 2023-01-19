// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

bool number(Instruction & obj, std::unordered_map<std::string, uint32_t> & data) {
    if (!obj.var) {
        return false;
    }

    // We never revalue a number, otherwise we might end up in a situation where
    // we change the number in the assignment, but then the users point to the
    // wrong thing
    if (obj.var.version > 0) {
        return false;
    }

    obj.var.version = ++data[obj.var.name];

    return true;
}

// Annotate usages of identifiers, so know if we need to replace them
bool number_uses(const uint32_t & index, Instruction & obj, LastSeenTable & tab) {
    bool progress = false;

    auto & table = tab[index];

    if (std::holds_alternative<Identifier>(*obj.obj_ptr)) {
        auto & id = std::get<Identifier>(*obj.obj_ptr);
        if (const auto & v = table.find(id.value); v != table.end()) {
            id.version = v->second;
        }
    } else if (std::holds_alternative<FunctionCall>(*obj.obj_ptr)) {
        const auto & func = std::get<FunctionCall>(*obj.obj_ptr);
        if (std::holds_alternative<Identifier>(*func.holder.obj_ptr)) {
            auto & id = std::get<Identifier>(*func.holder.obj_ptr);
            if (const auto & v = table.find(id.value); v != table.end()) {
                id.version = v->second;
            }
        }
        progress |= function_argument_walker(
            obj, [&](Instruction & i) { return number_uses(index, i, tab); });
    }

    if (obj.var) {
        table[obj.var.name] = obj.var.version;
    }

    return progress;
}

} // namespace

bool value_numbering(BasicBlock & block, std::unordered_map<std::string, uint32_t> & data) {
    return function_walker(block, {[&](Instruction & obj) { return number(obj, data); }});
}

bool usage_numbering(BasicBlock & block, LastSeenTable & table) {
    const auto number = [&](Instruction & obj) { return number_uses(block.index, obj, table); };

    table[block.index] = {};
    for (const auto & p : block.parents) {
        table[block.index].merge(table[p->index]);
    }

    return function_walker(block, number);
}

} // namespace MIR::Passes
