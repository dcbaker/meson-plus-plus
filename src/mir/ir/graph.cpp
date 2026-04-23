// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "graph.hpp"
#include "basicblock.hpp"
#include "helpers.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cassert>
#include <deque>
#include <set>
#include <sstream>

namespace MIR::IR {

size_t NodeHash::operator()(const Node & p) const { return p.id; }
size_t NodeHash::operator()(const Node * p) const { return p->id; }

Node::Node(uint32_t i, std::shared_ptr<BasicBlock> b, CFG * const cfg)
    : id{i}, depth{0}, block{b}, predecessors{}, successors{}, loop_header{false}, p_cfg{cfg} {};

Node * Node::left_successor() const { return successors.at(0); }
Node * Node::right_successor() const { return successors.at(1); }

// TODO: could this be implemented as iteration instead of recursion?
void update_depth(Node * node, const Node * const parent) {
    // I would need to implement a depth-free iterator to do this as iteration
    // instead of recursion. That may still be desirable.

    // If the successor is a loop header, we need to ensure that we are
    // not setting the depth to the loop body (right arm) depth + 1, instead
    // in that case the header should not be updated.
    //
    // If this is the node is a successor is a loop header, we check to see
    // if this node is a child of that successor, if it is we stop
    //
    // TODO: this algorithm sucks
    if (node->loop_header && node->successors.at(0)) {
        std::deque<Node *> queue{node->successors.at(0)};
        while (!queue.empty()) {
            const Node * const n = queue.front();
            queue.pop_front();
            if (*n == *parent) {
                return;
            }
            for (auto succ : n->successors) {
                if (succ && succ->depth > node->depth) {
                    queue.push_back(succ);
                }
            }
        }
    }

    if (node->depth <= parent->depth) {
        node->depth = parent->depth + 1;

        if (auto s = node->successors.at(0)) {
            update_depth(s, node);
        }
        if (auto s = node->successors.at(1)) {
            update_depth(s, node);
        }
    }
}

void Node::set_successor(Node * n, int index) {
    assert(index == 0 || index == 1);

    // We can replace the special tail node, but in that case we need to move it
    assert(successors.at(index) == nullptr || successors.at(index)->id == UINT32_MAX ||
           n == nullptr);

    if (n != nullptr) {
        // If the depth of the new successor is less than the depth of the current
        // node, increase that depth
        update_depth(n, this);

        // If we are replacing the left tail successor, we want to move that
        // successor to be the successor of the new block if it doesn't have one.
        // We do not walk down looking for later successors as we consider that the
        // callers job to handle
        if (successors.at(index) && successors.at(index)->id == UINT32_MAX) {
            assert(index == 0);
            if (!n->successors.at(0)) {
                n->set_left_successor(successors.at(index));
            }
        }
    }

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

Node::Iterator Node::begin() { return Iterator(this); }
Node::Iterator Node::end() { return Iterator(this->p_cfg->tail()); }

Node::Iterator::Iterator(pointer head)
    : p_current{head}, p_depth{head->depth}, p_queue{{head->id, {}}} {};

Node::Iterator::reference Node::Iterator::operator*() { return *p_current; }

Node::Iterator::pointer Node::Iterator::operator->() { return p_current; }

Node::Iterator Node::Iterator::operator++(int) {
    Iterator tmp = *this;
    ++(*this);
    return tmp;
}

Node::Iterator & Node::Iterator::operator++() {
    for (Node * succ : p_current->successors) {
        // Queue any successors if they have not already been queued
        if (succ && succ->depth > p_depth) {
            // Create the entry if it doesn't exist
            auto & deq = p_queue[succ->depth];
            if (std::find(deq.begin(), deq.end(), succ) == deq.end()) {
                deq.emplace_back(succ);
            }
        }
    }

#ifdef MESONPP_DEBUG
    if (p_depth > 0) {
        for (int64_t i = p_depth - 1; i > 0; --i) {
            assert(p_queue[i].empty());
        }
    }
#endif

    if (p_queue.at(p_depth).empty()) {
        p_depth++;
        // We should never have a case where a depth is empty, that's a bug
        assert(!p_queue[p_depth].empty());
    }

    p_current = p_queue.at(p_depth).front();
    p_queue.at(p_depth).pop_front();

    return *this;
}

bool operator==(const Node::Iterator & a, const Node::Iterator & b) {
    return a.p_current == b.p_current;
}

bool operator!=(const Node::Iterator & a, const Node::Iterator & b) {
    return a.p_current != b.p_current;
}

Node::RIterator Node::rbegin() { return RIterator(this->p_cfg->tail()); }
Node::RIterator Node::rend() { return RIterator(this); }

Node::RIterator::RIterator(pointer head)
    : p_current{head}, p_depth{head->depth}, p_queue{{head->id, {}}} {};

Node::RIterator::reference Node::RIterator::operator*() { return *p_current; }

Node::RIterator::pointer Node::RIterator::operator->() { return p_current; }

Node::RIterator Node::RIterator::operator++(int) {
    RIterator tmp = *this;
    ++(*this);
    return tmp;
}

// It's annoying how much of this is copied from the forward iterator
Node::RIterator & Node::RIterator::operator++() {
    for (Node * succ : p_current->successors) {
        // Queue any successors if they have not already been queued
        if (succ && succ->depth < p_depth) {
            // Create the entry if it doesn't exist
            auto & deq = p_queue[succ->depth];
            if (std::find(deq.begin(), deq.end(), succ) == deq.end()) {
                deq.emplace_back(succ);
            }
        }
    }

#ifdef MESONPP_DEBUG
    if (p_depth < 0) {
        for (int64_t i = p_depth + 1; i > 0; ++i) {
            assert(p_queue[i].empty());
        }
    }
#endif

    if (p_queue.at(p_depth).empty()) {
        p_depth--;
        // We should never have a case where a depth is empty, that's a bug
        assert(!p_queue[p_depth].empty());
    }

    p_current = p_queue.at(p_depth).front();
    p_queue.at(p_depth).pop_front();

    return *this;
}

bool operator==(const Node::RIterator & a, const Node::RIterator & b) {
    return a.p_current == b.p_current;
}

bool operator!=(const Node::RIterator & a, const Node::RIterator & b) {
    return a.p_current != b.p_current;
}

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

Node::Iterator CFG::begin() { return head()->begin(); }
Node::Iterator CFG::end() { return head()->end(); }

Node::RIterator CFG::rbegin() { return head()->rbegin(); }
Node::RIterator CFG::rend() { return head()->rend(); }

CFG::CFG() : nodes{}, p_block_counter{0} {
    nodes.emplace(p_block_counter,
                  std::make_unique<Node>(p_block_counter, std::make_shared<BasicBlock>(), this));
    nodes.emplace(UINT32_MAX, std::make_unique<Node>(UINT32_MAX, nullptr, this));
}

Node * CFG::head() const { return nodes.at(0).get(); }
Node * CFG::tail() const { return nodes.at(UINT32_MAX).get(); }

Node * CFG::next() {
    assert(p_block_counter != UINT32_MAX);
    const uint32_t i = ++p_block_counter;
    nodes.emplace(i, std::make_unique<Node>(i, std::make_shared<BasicBlock>(), this));
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
