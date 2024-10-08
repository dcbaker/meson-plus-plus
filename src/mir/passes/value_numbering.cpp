// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

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
    if (obj.var.gvn > 0) {
        return false;
    }

    obj.var.gvn = ++data[obj.var.name];

    return true;
}

} // namespace

bool value_numbering(BasicBlock & block, std::unordered_map<std::string, uint32_t> & data) {
    return function_walker(block, {[&](Instruction & obj) { return number(obj, data); }});
}

// Annotate usages of identifiers, so know if we need to replace them
bool UsageNumbering::number_instructions(Instruction & obj, const uint32_t index) {
    bool progress = false;

    auto & table = data[index];

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
            obj, [&index, this](Instruction & i) { return this->number_instructions(i, index); });
    }

    if (obj.var) {
        table[obj.var.name] = obj.var.gvn;
    }

    return progress;
}

bool UsageNumbering::operator()(BasicBlock & block) {
    data[block.index] = {};
    for (auto && p : block.parents) {
        data[block.index].merge(data[p->index]);
    }
    return function_walker(block, [this, &block](Instruction & obj) {
        return this->number_instructions(obj, block.index);
    });
}

} // namespace MIR::Passes
