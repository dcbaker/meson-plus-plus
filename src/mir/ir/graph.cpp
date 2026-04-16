// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "graph.hpp"
#include "basicblock.hpp"

#include <cassert>
#include <deque>
#include <set>
#include <sstream>

namespace MIR::IR {

namespace {

uint32_t node_id_base = 0;
Node node_sentintel = Node{UINT32_MAX, nullptr};

} // namespace

Node::Node()
    : id{node_id_base++}, block{std::make_shared<BasicBlock>()}, predecessors{}, loop_header{false},
      successors{}, children{} {};
Node::Node(std::shared_ptr<BasicBlock> b)
    : id{node_id_base++}, block{b}, predecessors{}, loop_header{false}, successors{}, children{} {};
Node::Node(uint32_t i, std::shared_ptr<BasicBlock> b)
    : id{i}, block{b}, predecessors{}, loop_header{false}, successors{}, children{} {};

std::shared_ptr<Node> Node::left_successor() const { return successors[0].lock(); }
std::shared_ptr<Node> Node::right_successor() const { return successors[1].lock(); }

bool Node::has_successor(int index) const {
    assert(index == 0 || index == 1);
    return !!successors.at(index).lock();
}

void Node::set_successor(std::shared_ptr<Node> n, int index) {
    assert(index == 0 || index == 1);
    assert(!has_successor(index));

    successors.at(index) = n;
    if (!n->loop_header) {
        children.at(index) = n;
    }
}

void Node::set_right_successor(std::shared_ptr<Node> n) {
    set_successor(std::forward<std::shared_ptr<Node>>(n), 1);
}

void Node::set_left_successor(std::shared_ptr<Node> n) {
    set_successor(std::forward<std::shared_ptr<Node>>(n), 0);
}

std::string Node::serialize() const {
    std::stringstream ss{};
    ss << "Node {\n"
       << "  id = { " << id << " }\n"
       << "  loop_header = { " << (loop_header ? "true" : "false") << " }\n"
       << "  block = {\n"
       << block->serialize() << "\n}"
       << "}";

    return ss.str();
}

bool Node::operator==(const Node & other) const { return this->id == other.id; }

bool Node::operator!=(const Node & other) const { return this->id != other.id; }

Node::Iterator Node::begin() { return Node::Iterator(this); }
Node::Iterator Node::end() { return Node::Iterator(&node_sentintel); }

Node::Iterator::Iterator(pointer ptr) {
    deque.push_back(ptr);
    processed.emplace(ptr->id);
}

void link_nodes(std::shared_ptr<Node> pred, std::shared_ptr<Node> succ, bool right) {
    if (right) {
        pred->set_right_successor(succ);
    } else {
        pred->set_left_successor(succ);
    }
    succ->predecessors.push_back(pred);
}

Node::Iterator::reference Node::Iterator::operator*() const { return *deque.front(); }

Node::Iterator::pointer Node::Iterator::operator->() { return deque.front(); }

Node::Iterator & Node::Iterator::operator++() {
    Node * root = deque.front();

    if (root->has_successor(0) || root->has_successor(1)) {
        if (root->has_successor(0)) {
            auto n = root->left_successor();
            if (processed.find(n->id) == processed.end()) {
                deque.push_back(n.get());
                processed.emplace(n->id);
            }
        }
        if (root->has_successor(1)) {
            auto n = root->right_successor();
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

std::string CFG::serialize() const { return root->serialize(); }

Node::Iterator CFG::begin() { return root->begin(); }
Node::Iterator CFG::end() { return root->end(); }

} // namespace MIR::IR
