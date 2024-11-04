// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include "passes.hpp"
#include "private.hpp"

#include <algorithm>
#include <cassert>
#include <tuple>

namespace MIR::Passes {

bool GlobalValueNumbering::insert_phis(BasicBlock & b) {
    // Merge the data down, even for strictly dominated blocks
    for (auto && p : b.predecessors) {
        for (auto && [var, value] : data[p->index]) {
            if (data[b.index].find(var) == data[b.index].end()) {
                data[b.index][var] = value;
            } else {
                data[b.index][var] = std::max(data[b.index][var], value);
            }
        }
    }

    if (b.predecessors.size() <= 1) {
        return false;
    }

    // Calculate all variables that have convergence
    //
    // This is true if a variable is present in at least two predecessors
    std::vector<std::tuple<std::string, std::vector<uint32_t>>> convergence{};
    for (auto && [var, _] : data[b.index]) {
        std::vector<uint32_t> values;
        for (auto && p : b.predecessors) {
            if (auto && i = data[p->index].find(var); i != data[p->index].end()) {
                values.emplace_back(i->second);
            }
        }
        if (values.size() >= 2) {
            convergence.emplace_back(std::make_tuple(var, std::move(values)));
        }
    }

    if (convergence.empty()) {
        return false;
    }

    std::list<Instruction> phis;
    for (auto && [var, values] : convergence) {
        auto it = values.begin();
        uint32_t cur = ++data[b.index][var];
        uint32_t prev = *it++;
        for (; it != values.end(); ++it) {
            phis.emplace_back(Instruction{Phi{prev, *it}, Variable{var, cur}});
            prev = cur++;
        }
    }

    b.instructions.splice(b.instructions.begin(), phis);

    return true;
};

bool GlobalValueNumbering::number(Instruction & obj, const uint32_t block_index) {
    bool progress = false;
    std::unordered_map<std::string, uint32_t> & table = data[block_index];

    if (std::holds_alternative<Identifier>(*obj.obj_ptr)) {
        auto & id = std::get<Identifier>(*obj.obj_ptr);
        // TODO: use before definition
        if (!id.version) {
            id.version = table.at(id.value);
            progress = true;
        }
    } else if (std::holds_alternative<Array>(*obj.obj_ptr)) {
        progress |=
            array_walker(obj, {[&](Instruction & i) { return this->number(i, block_index); }});
    } else if (std::holds_alternative<FunctionCall>(*obj.obj_ptr)) {
        auto && fc = std::get<FunctionCall>(*obj.obj_ptr);
        progress |= number(fc.holder, block_index);
        progress |= function_argument_walker(
            obj, {[&](Instruction & i) { return this->number(i, block_index); }});
    }
    // TODO: dict

    // This needs to be done after numbering array and dict members, and
    // function arguments, which might otherwise create a circular reference
    if (obj.var && obj.var.gvn == 0) {
        obj.var.gvn = ++gvn[obj.var.name];
        table[obj.var.name] = obj.var.gvn;
        progress = true;
    }

    return progress;
}

bool GlobalValueNumbering::operator()(BasicBlock & block) {
    // Don't run this pass on the same data twice
    if (data.find(block.index) != data.end()) {
        return false;
    }
    data[block.index] = {};

    bool progress = false;
    progress |= insert_phis(block);
    progress |= instruction_walker(
        block, {[this, &block](Instruction & i) { return number(i, block.index); }});
    return progress;
}

} // namespace MIR::Passes
