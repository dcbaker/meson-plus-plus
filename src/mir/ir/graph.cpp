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

namespace {

void update_depth(Node * node, const Node * const parent) {
    // I would need to implement a depth-free iterator to do this as iteration
    // instead of recursion. That may still be desirable.

    // The caller must sort after finishing all depth updates

    // If the successor is a loop header, we need to ensure that we are
    // not setting the depth to the loop body (right arm) depth + 1, instead
    // in that case the header should not be updated.
    //
    // If this is the node is a successor is a loop header, we check to see
    // if this node is a child of that successor, if it is we stop
    //
    // TODO: this algorithm sucks
    if (auto s = node->successors.at(0); node->loop_header && s) {
        std::deque<Node *> queue{s};
        while (!queue.empty()) {
            Node * n = queue.front();
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
        for (auto && s : node->successors) {
            if (s) {
                update_depth(s, node);
            }
        }
    }
}

} // namespace

size_t NodeHash::operator()(const Node * const node) const { return node->m_const_id; }

Node::Node(uint32_t const_id, uint32_t id, std::shared_ptr<BasicBlock> b, CFG * const cfg)
    : m_const_id{const_id}, id{id}, depth{0}, block{b}, predecessors{}, successors{},
      loop_header{false}, p_cfg{cfg} {};

Node * Node::get_successor(int index) const {
    assert(index == 0 || index == 1);
    return successors.at(index);
}

Node * Node::left_successor() const { return get_successor(0); }

Node * Node::right_successor() const { return get_successor(1); }

void Node::set_successor(Node * n, int index) {
    assert(index == 0 || index == 1);

    // We can replace the special tail node, but in that case we need to move it
    assert(!successors.at(index) || n == nullptr);

    if (n != nullptr) {
        // If the depth of the new successor is less than the depth of the current
        // node, increase that depth
        update_depth(n, this);
    }

    successors.at(index) = n;
    p_cfg->sort();
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

bool Node::operator<(const Node & other) const { return depth < other.depth; }

CFG::NodeVec::iterator Node::begin() {
    // Because the storage is flat, even when we run forward to this index + 1,
    // we can still be returning blocks with the same depth as the start block,
    // which is incorrect.
    auto itr = std::next(p_cfg->begin(), id);
    while ((*itr)->depth <= depth) {
        itr = std::next(itr);
    }
    return itr;
}
CFG::NodeVec::iterator Node::end() { return p_cfg->end(); }

CFG::NodeVec::const_iterator Node::cbegin() const {
    auto itr = std::next(p_cfg->cbegin(), id);
    while ((*itr)->depth <= depth) {
        itr = std::next(itr);
    }
    return itr;
}
CFG::NodeVec::const_iterator Node::cend() const { return p_cfg->cend(); }

CFG::NodeVec::reverse_iterator Node::rbegin() {
    // we have the distance from the start of the vector, but we need to get the
    // distance from the back
    const uint64_t distance = p_cfg->nodes.size() - id;
    return std::next(p_cfg->rbegin(), distance);
}
CFG::NodeVec::reverse_iterator Node::rend() { return p_cfg->rend(); }

CFG::NodeVec::const_reverse_iterator Node::crbegin() const {
    const uint64_t distance = p_cfg->nodes.size() - id;
    return std::next(p_cfg->crbegin(), distance);
}
CFG::NodeVec::const_reverse_iterator Node::crend() const { return p_cfg->crend(); }

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

CFG::CFG() : nodes{}, p_const_ids{0} {
    nodes.emplace_back(std::make_unique<Node>(p_next_const_id(), nodes.size(),
                                              std::make_shared<BasicBlock>(), this));
}

uint32_t CFG::p_next_const_id() { return p_const_ids++; }

Node * CFG::head() const { return nodes.at(0).get(); }

Node * CFG::next() {
    auto & v = nodes.emplace_back(std::make_unique<Node>(p_next_const_id(), nodes.size(),
                                                         std::make_shared<BasicBlock>(), this));
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
