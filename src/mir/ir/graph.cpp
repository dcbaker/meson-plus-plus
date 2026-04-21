// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "graph.hpp"
#include "basicblock.hpp"
#include "helpers.hpp"

#include <cassert>
#include <deque>
#include <set>
#include <sstream>

namespace MIR::IR {

namespace {

Node node_sentintel = Node{UINT32_MAX, nullptr};

} // namespace

size_t NodeHash::operator()(const Node & p) const { return p.id; }
size_t NodeHash::operator()(const Node * p) const { return p->id; }

Node::Node(uint32_t i, std::shared_ptr<BasicBlock> b)
    : id{i}, block{b}, predecessors{}, successors{}, loop_header{false} {};

Node * Node::left_successor() const { return successors[0]; }
Node * Node::right_successor() const { return successors[1]; }

void Node::set_successor(Node * n, int index) {
    assert(index == 0 || index == 1);
    assert(successors.at(index) == nullptr || n == nullptr);

    successors.at(index) = n;
}

void Node::set_right_successor(Node * n) { set_successor(std::forward<Node *>(n), 1); }

void Node::set_left_successor(Node * n) { set_successor(std::forward<Node *>(n), 0); }

std::string Node::serialize(unsigned indent) const {
    const std::string ind = Private::indenter(indent + 1);

    std::stringstream ss{};
    ss << Private::indenter(indent) << "Node {\n"
       << ind << "id = { " << id << " }\n"
       << ind << "loop_header = { " << (loop_header ? "true" : "false") << " }\n"
       << ind << "predecessors = {";

    for (auto && p : predecessors) {
        ss << " " << p->id;
    }
    ss << " }\n";

    ss << ind << "successors = {";
    for (auto && succ : successors) {
        if (succ) {
            ss << " " << succ->id;
        }
    }
    ss << " }\n";

    ss << ind << "block = {\n" << block->serialize(indent + 2) << "\n" << ind << "}\n" << "}";

    return ss.str();
}

bool Node::operator==(const Node & other) const { return this->id == other.id; }

bool Node::operator!=(const Node & other) const { return this->id != other.id; }

void link_nodes(Node * pred, Node * succ, bool right) {
    if (right) {
        pred->set_right_successor(succ);
    } else {
        pred->set_left_successor(succ);
    }
    succ->predecessors.emplace(pred);
}

void reparent(Node * from, Node * to) {
    if (auto s = from->left_successor()) {
        s->predecessors.erase(from);
        s->predecessors.emplace(to);
        to->set_left_successor(s);
        from->set_left_successor(nullptr);
    }

    if (auto s = from->right_successor()) {
        s->predecessors.erase(from);
        s->predecessors.emplace(to);
        to->set_right_successor(s);
        from->set_right_successor(nullptr);
    }
}

bool operator==(const CFG::Iterator & a, const CFG::Iterator & b) {
    return a.deque.front() == b.deque.front();
}

bool operator!=(const CFG::Iterator & a, const CFG::Iterator & b) {
    return a.deque.front() != b.deque.front();
}

CFG::Iterator::Iterator(pointer ptr, pointer tail) : p_tail{tail} {
    deque.push_back(ptr);
    queued.emplace(ptr->id);
}

CFG::Iterator::reference CFG::Iterator::operator*() { return *deque.front(); }

CFG::Iterator::pointer CFG::Iterator::operator->() { return deque.front(); }

CFG::Iterator & CFG::Iterator::operator++() {
    pointer current = deque.front();
    visited.emplace(current->id);

    // Queue the nodes successors
    // Do this after visiting the node in case it's successors are mutated during
    // that visit
    for (auto succ : {current->left_successor(), current->right_successor()}) {
        if (succ && queued.find(succ->id) == queued.end()) {
            deque.push_back(succ);
            queued.emplace(succ->id);
        }
    }

    // We are done with this node, advance to the next one
    deque.pop_front();

    // If the next node (the front of the deque) has parents that have not been
    // visited, then we can't use that one yet, push it to the back of the queue
    // and take the next one until we've visited them all
    if (!deque.empty()) {
        bool cont;
        do {
            cont = false;
            pointer value = deque.front();

            if (value) {
                for (auto && pred : value->predecessors) {
                    // This happens to work, but seems fragile
                    if (visited.find(pred->id) == visited.end() && !value->loop_header) {
                        deque.pop_front();
                        deque.push_back(value);
                        cont = true;
                        break;
                    }
                }
            }
        } while (cont);
    } else {
        // If the queue is empty, we've (hopefully) reached the end of the
        // graph, and we'll push the tail sentinel on there.
        deque.push_back(p_tail);
    }

    return *this;
}

CFG::Iterator CFG::Iterator::operator++(int) {
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

CFG::Iterator CFG::begin() { return Iterator(head(), tail()); }
CFG::Iterator CFG::end() { return Iterator(tail(), tail()); }

CFG::CFG() : nodes{}, p_block_counter{0} {
    nodes.emplace(p_block_counter,
                  std::make_unique<Node>(p_block_counter, std::make_shared<BasicBlock>()));
    nodes.emplace(UINT32_MAX, std::make_unique<Node>(UINT32_MAX, nullptr));
}

Node * CFG::head() const { return nodes.at(0).get(); }
Node * CFG::tail() const { return nodes.at(UINT32_MAX).get(); }

Node * CFG::next() {
    assert(p_block_counter != UINT32_MAX);
    const uint32_t i = ++p_block_counter;
    nodes.emplace(i, std::make_unique<Node>(i, std::make_shared<BasicBlock>()));
    return nodes.at(i).get();
}

std::string CFG::serialize(unsigned indent) {
    std::stringstream ss{};
    for (auto n : *this) {
        ss << n.serialize(indent) << "\n\n";
    }
    return ss.str();
}

} // namespace MIR::IR
