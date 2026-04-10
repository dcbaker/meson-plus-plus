// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "builder.hpp"
#include "ir.hpp"

#include <cassert>

namespace MIR::Builder {

Builder::Builder() : p_root{std::make_shared<IR::Node>()} {};

std::shared_ptr<IR::Node> Builder::finalize() {
    if (p_condition) {
        p_root->block->instructions.emplace_back(std::move(p_condition));
    }
    return p_root;
}

Builder & Builder::add_inst(std::unique_ptr<IR::Instruction> && inst) {
    assert(inst != nullptr);
    p_root->block->instructions.emplace_back(std::move(inst));
    return *this;
}

Builder & Builder::add_condition(std::unique_ptr<IR::Instruction> && inst) {
    p_condition = std::move(inst);
    return *this;
}

Builder Builder::left_successor() {
    assert(p_root->successors[0] == nullptr);
    Builder b{};
    IR::link_nodes(p_root, b.p_root);
    return b;
}

Builder Builder::right_successor() {
    assert(p_root->successors[1] == nullptr);
    Builder b{};
    IR::link_nodes(p_root, b.p_root, true);
    return b;
}

Builder & Builder::link_left_successor(Builder & b) { return link_left_successor(b.p_root); }

Builder & Builder::link_left_successor(std::shared_ptr<IR::Node> node) {
    assert(p_root->successors[0] == nullptr);
    IR::link_nodes(p_root, node);
    return *this;
}

Builder & Builder::link_right_successor(Builder & b) { return link_right_successor(b.p_root); }

Builder & Builder::link_right_successor(std::shared_ptr<IR::Node> node) {
    assert(p_root->successors[1] == nullptr);
    IR::link_nodes(p_root, node);
    return *this;
}

} // namespace MIR::Builder
