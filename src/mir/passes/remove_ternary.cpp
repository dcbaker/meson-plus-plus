// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "remove_ternary.hpp"
#include "builder/builder.hpp"
#include "ir/functioncall.hpp"

#include <algorithm>

namespace MIR::Passes {

bool remove_ternary(IR::CFG * cfg, IR::BasicBlock * node) {
    auto && test_func = [](const std::unique_ptr<IR::Instruction> & i) -> bool {
        if (std::holds_alternative<std::shared_ptr<IR::FunctionCall>>(i->instruction)) {
            auto f = std::get<std::shared_ptr<IR::FunctionCall>>(i->instruction);
            return f->m_func_id == IR::FunctionId::ternary;
        }
        return false;
    };

    bool progress = false;

    while (true) {
        auto & insts = node->instructions;
        auto itr = std::find_if(insts.begin(), insts.end(), test_func);
        if (itr == insts.end()) {
            break;
        }
        progress = true;

        /* A ternary is a condition followed by a variable set to a true value
         * or a false value, this is better represented as a condition and
         * blocks. In fact, to get into valid SSA form we need to get rid of the ternary.
         *
         * We'll end up with a form like this:
         *
         *                     O current block
         *                    / \
         *                   O   O
         *                    \ /
         *                     O tail
         */

        // Remove the ternary instruction from the instruction list
        std::unique_ptr<IR::Instruction> ternary = std::move(*itr);
        auto ternary_func = std::get<std::shared_ptr<IR::FunctionCall>>(ternary->instruction);
        itr = insts.erase(itr);

        // put the instructions following the ternary into the new block
        builder::Builder tail{cfg};
        auto & tail_insts = tail.get()->instructions;
        tail_insts.splice(tail_insts.end(), insts, itr, insts.end());

        // Give the original node's successors to the tail
        // FIXME: this gets us compiling, but it's wrong
        reparent(node, tail.get());

        // deconstruct the ternary, making the condition of the ternary the
        // condition of the original block, and constructing two new blocks
        // for the values, and then returning to the tail
        builder::Builder node_b{cfg, node};
        node_b.add_condition(
            std::make_unique<IR::Instruction>(std::move(ternary_func->m_pos.at(0))));

        builder::Builder lhs = node_b.left_successor();
        lhs.link_left_successor(tail);
        auto v = std::make_unique<IR::Instruction>(std::move(ternary_func->m_pos.at(1)));
        v->variable = ternary->variable;
        lhs.add_inst(std::move(v));

        builder::Builder rhs = node_b.right_successor();
        rhs.link_left_successor(tail);
        v = std::make_unique<IR::Instruction>(std::move(ternary_func->m_pos.at(2)));
        v->variable = ternary->variable;
        rhs.add_inst(std::move(v));
    }
    return progress;
}

} // namespace MIR::Passes
