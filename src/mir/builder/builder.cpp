// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "builder.hpp"
#include "ir.hpp"

#include <cassert>

namespace MIR::builder {

Builder::Builder()
    : p_root{std::make_shared<IR::Node>()}, p_cursor{p_root->block->instructions.begin()} {};

Builder::Builder(std::shared_ptr<IR::Node> node)
    : p_root{node}, p_cursor{p_root->block->instructions.begin()} {
    // Put the condition into the Builder, put it back with finalize
    set_cursor_end();
};

std::shared_ptr<IR::Node> Builder::get() const { return p_root; }

Builder & Builder::add_inst(std::unique_ptr<IR::Instruction> && inst) {
    assert(inst);
    p_cursor = p_root->block->instructions.emplace(p_cursor, std::move(inst));
    // Set the cursor forward one so that we write instructions after the one we
    // just inserted
    ++p_cursor;
    return *this;
}

Builder & Builder::add_condition(std::unique_ptr<IR::Instruction> && inst) {
    inst->m_is_block_condition = true;
    p_cursor =
        p_root->block->instructions.emplace(p_root->block->instructions.end(), std::move(inst));
    return *this;
}

Builder Builder::left_successor() {
    Builder b{};
    link_left_successor(b);
    return b;
}

Builder Builder::right_successor() {
    Builder b{};
    link_right_successor(b);
    return b;
}

Builder & Builder::link_left_successor(std::shared_ptr<Builder> & b) {
    return link_left_successor(b->p_root);
}

Builder & Builder::link_left_successor(Builder & b) { return link_left_successor(b.p_root); }

Builder & Builder::link_left_successor(std::shared_ptr<IR::Node> node) {
    assert(!p_root->successors[0]);
    IR::link_nodes(p_root, node);
    return *this;
}

Builder & Builder::link_right_successor(std::shared_ptr<Builder> & b) {
    return link_right_successor(b->p_root);
}

Builder & Builder::link_right_successor(Builder & b) { return link_right_successor(b.p_root); }

Builder & Builder::link_right_successor(std::shared_ptr<IR::Node> node) {
    assert(!p_root->successors[1]);
    IR::link_nodes(p_root, node, true);
    return *this;
}

Builder & Builder::set_cursor_begin() {
    p_cursor = p_root->block->instructions.begin();
    return *this;
}

Builder & Builder::set_cursor_end() {
    if (!p_root->block->instructions.empty()) {
        p_cursor = p_root->block->instructions.end();

        // If the last instruction is a condition, then set the cursor before
        // that
        auto prev = --p_root->block->instructions.end();
        if ((*prev)->m_is_block_condition) {
            p_cursor = prev;
        }
    }
    return *this;
}

} // namespace MIR::builder
