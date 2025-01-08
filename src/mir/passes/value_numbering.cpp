// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include "exceptions.hpp"
#include "passes.hpp"
#include "private.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>
#include <tuple>

namespace MIR::Passes {

bool GlobalValueNumbering::insert_phis(CFGNode & b) {
    // Merge the data down, even for strictly dominated blocks
    for (auto && r : b.predecessors) {
        auto p = r.lock();
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
        for (auto && r : b.predecessors) {
            auto p = r.lock();
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

    std::list<Object> phis;
    for (auto && [var, values] : convergence) {
        auto it = values.begin();
        uint32_t cur = ++data[b.index][var];
        uint32_t prev = *it++;
        for (; it != values.end(); ++it) {
            auto phi = std::make_shared<Phi>(prev, *it);
            phi->var = Variable{var, cur};
            phis.emplace_back(phi);
            prev = cur++;
        }
    }

    b.block->instructions.splice(b.block->instructions.begin(), phis);

    return true;
};

bool GlobalValueNumbering::number(Object & obj, const uint32_t block_index) {
    bool progress = false;
    std::unordered_map<std::string, uint32_t> & table = data[block_index];

    if (std::holds_alternative<IdentifierPtr>(obj)) {
        auto & id = std::get<IdentifierPtr>(obj);
        // TODO: use before definition
        if (!id->version) {
            try {
                id->version = table.at(id->value);
            } catch (std::out_of_range &) {
                throw Util::Exceptions::MesonException{"Attempted to use variable '" + id->value +
                                                       "' before it's definition"};
            }
            progress = true;
        }
    }

    // This needs to be done after numbering array and dict members, and
    // function arguments, which might otherwise create a circular reference
    MIR::Variable & var = std::visit(VariableGetter{}, obj);
    if (var && var.gvn == 0) {
        var.gvn = ++gvn[var.name];
        table[var.name] = var.gvn;
        progress = true;
    }

    return progress;
}

bool GlobalValueNumbering::operator()(std::shared_ptr<CFGNode> block) {
    // Don't run this pass on the same data twice
    if (data.find(block->index) != data.end()) {
        return false;
    }
    data[block->index] = {};

    bool progress = false;
    progress |= insert_phis(*block);
    progress |= instruction_walker(
        *block, {[this, &block](Object & i) { return number(i, block->index); }});
    return progress;
}

} // namespace MIR::Passes
