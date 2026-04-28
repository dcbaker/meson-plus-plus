// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "graph.hpp"
#include "helpers.hpp"

#include <algorithm>
#include <sstream>

namespace MIR::IR {

CFG::CFG() : nodes{}, p_const_ids{0} {
    nodes.emplace_back(std::make_unique<Node>(p_next_const_id(), nodes.size(), this));
}

uint32_t CFG::p_next_const_id() { return p_const_ids++; }

Node * CFG::head() const { return nodes.at(0).get(); }

Node * CFG::next() {
    auto & v = nodes.emplace_back(std::make_unique<Node>(p_next_const_id(), nodes.size(), this));
    return v.get();
}

void CFG::sort() {
    std::sort(
        nodes.begin(), nodes.end(),
        [](const std::unique_ptr<Node> & i, const std::unique_ptr<Node> & j) { return *i < *j; });
    for (uint64_t i = 0; i < nodes.size(); ++i) {
        nodes.at(i)->id = i;
    }
}

CFG::NodeVec::iterator CFG::begin() { return nodes.begin(); }
CFG::NodeVec::iterator CFG::end() { return nodes.end(); }

CFG::NodeVec::const_iterator CFG::cbegin() const { return nodes.cbegin(); }
CFG::NodeVec::const_iterator CFG::cend() const { return nodes.cend(); }

CFG::NodeVec::reverse_iterator CFG::rbegin() { return nodes.rbegin(); }
CFG::NodeVec::reverse_iterator CFG::rend() { return nodes.rend(); }

CFG::NodeVec::const_reverse_iterator CFG::crbegin() const { return nodes.crbegin(); }
CFG::NodeVec::const_reverse_iterator CFG::crend() const { return nodes.crend(); }

std::string CFG::serialize(unsigned indent) {
    std::stringstream ss{};
    for (auto && n : *this) {
        ss << n->serialize(indent) << "\n\n";
    }
    return ss.str();
}

} // namespace MIR::IR
