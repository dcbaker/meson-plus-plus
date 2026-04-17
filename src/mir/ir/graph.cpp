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

uint32_t node_id_base = 0;
Node node_sentintel = Node{UINT32_MAX, nullptr};
std::shared_ptr<Node> node_sentintel_ptr = std::make_shared<Node>(UINT32_MAX, nullptr);

} // namespace

Predecessor::Predecessor(std::shared_ptr<Node> n) : m_p{n}, m_id{n->id} {};

bool Predecessor::operator==(const Predecessor & other) const { return m_id == other.m_id; }

size_t PredecessorHash::operator()(const Predecessor & p) const { return p.m_id; }

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
    // we do allow a success here if we're unsetting the successor
    assert(!has_successor(index) || !n);

    successors.at(index) = n;
    if (!n || !n->loop_header) {
        children.at(index) = n;
    }
}

void Node::set_right_successor(std::shared_ptr<Node> n) {
    set_successor(std::forward<std::shared_ptr<Node>>(n), 1);
}

void Node::set_left_successor(std::shared_ptr<Node> n) {
    set_successor(std::forward<std::shared_ptr<Node>>(n), 0);
}

std::string Node::serialize(unsigned indent) const {
    const std::string ind = Private::indenter(indent + 1);

    std::stringstream ss{};
    ss << Private::indenter(indent) << "Node {\n"
       << ind << "id = { " << id << " }\n"
       << ind << "loop_header = { " << (loop_header ? "true" : "false") << " }\n"
       << ind << "predecessors = {";

    for (auto && p : predecessors) {
        ss << " " << p.m_id;
    }
    ss << " }\n";

    ss << ind << "successors = {";
    for (auto && s : successors) {
        auto succ = s.lock();
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

void link_nodes(std::shared_ptr<Node> pred, std::shared_ptr<Node> succ, bool right) {
    if (right) {
        pred->set_right_successor(succ);
    } else {
        pred->set_left_successor(succ);
    }
    succ->predecessors.emplace(pred);
}

void reparent(std::shared_ptr<Node> from, std::shared_ptr<Node> to) {
    if (auto s = from->left_successor()) {
        s->predecessors.erase(from);
        s->predecessors.emplace(to);
        to->set_left_successor(s);
        from->set_left_successor(std::shared_ptr<IR::Node>(nullptr));
    }

    if (auto s = from->right_successor()) {
        s->predecessors.erase(from);
        s->predecessors.emplace(to);
        to->set_right_successor(s);
        from->set_right_successor(std::shared_ptr<IR::Node>(nullptr));
    }
}

bool operator==(const CFG::Iterator & a, const CFG::Iterator & b) {
    return a.deque.front() == b.deque.front();
}

bool operator!=(const CFG::Iterator & a, const CFG::Iterator & b) {
    return a.deque.front() != b.deque.front();
}

CFG::Iterator::Iterator(value_type value) {
    deque.push_back(value);
    queued.emplace(value->id);
}

CFG::Iterator::reference CFG::Iterator::operator*() { return deque.front(); }

CFG::Iterator::pointer CFG::Iterator::operator->() { return &deque.front(); }

CFG::Iterator & CFG::Iterator::operator++() {
    value_type current = deque.front();
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
            value_type value = deque.front();

            if (value) {
                for (auto && p : value->predecessors) {
                    auto pred = p.m_p.lock();
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
        deque.push_back(node_sentintel_ptr);
    }

    return *this;
}

CFG::Iterator CFG::Iterator::operator++(int) {
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

CFG::Iterator CFG::begin() { return Iterator(root); }
CFG::Iterator CFG::end() { return Iterator(node_sentintel_ptr); }

CFG::CFG(std::shared_ptr<Node> r) : root{r} {};

std::string CFG::serialize(unsigned indent) {
    std::stringstream ss{};
    for (auto n : *this) {
        ss << n->serialize(indent) << "\n\n";
    }
    return ss.str();
}

} // namespace MIR::IR
