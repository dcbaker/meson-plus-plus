// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "graph.hpp"

#include <deque>
#include <set>

namespace MIR::IR {

namespace {

uint32_t node_id_base = 0;
Node node_sentintel = Node{UINT32_MAX, nullptr};

} // namespace

Node::Node(std::shared_ptr<BasicBlock> b) : id{node_id_base++}, block{std::move(b)} {};
Node::Node(uint32_t i, std::shared_ptr<BasicBlock> b) : id{i}, block{b} {};

bool Node::operator==(const Node & other) const { return this->id == other.id; }

bool Node::operator!=(const Node & other) const { return this->id != other.id; }

Node::Iterator Node::begin() { return Node::Iterator(this); }
Node::Iterator Node::end() { return Node::Iterator(&node_sentintel); }

Node::Iterator::Iterator(pointer ptr) {
    deque.push_back(ptr);
    processed.emplace(ptr->id);
}

void link_nodes(std::shared_ptr<Node> pred, std::shared_ptr<Node> succ) {
    pred->successors.push_back(succ);
    succ->predecessors.push_back(pred);
}

Node::Iterator::reference Node::Iterator::operator*() const { return *deque.front(); }

Node::Iterator::pointer Node::Iterator::operator->() { return deque.front(); }

Node::Iterator & Node::Iterator::operator++() {
    Node * root = deque.front();

    if (!root->successors.empty()) {
        for (auto & n : root->successors) {
            if (processed.find(n->id) == processed.end()) {
                deque.push_back(n.get());
                processed.emplace(n->id);
            }
        }
    } else {
        deque.push_back(&node_sentintel);
        processed.emplace(node_sentintel.id);
    }

    deque.pop_front();
    return *this;
}

Node::Iterator Node::Iterator::operator++(int) {
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

CFG::CFG(std::shared_ptr<Node> r) : root{r} {};

Node::Iterator CFG::begin() { return root->begin(); }
Node::Iterator CFG::end() { return root->end(); }

} // namespace MIR
