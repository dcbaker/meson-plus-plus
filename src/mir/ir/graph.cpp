// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "graph.hpp"
#include "helpers.hpp"
#include "instruction.hpp"
#include "utils.hpp"

#include <algorithm>
#include <cassert>
#include <deque>
#include <list>
#include <sstream>

namespace MIR::IR {

namespace {

void update_depth(BasicBlock * block, const BasicBlock * const parent) {
    // I would need to implement a depth-free iterator to do this as iteration
    // instead of recursion. That may still be desirable.

    // The caller must sort after finishing all depth updates

    // If the successor is a loop header, we need to ensure that we are
    // not setting the depth to the loop body (right arm) depth + 1, instead
    // in that case the header should not be updated.
    //
    // If this is the block is a successor is a loop header, we check to see
    // if this block is a child of that successor, if it is we stop
    //
    // TODO: this algorithm sucks
    if (auto s = block->successors.at(0); block->loop_header && s) {
        std::deque<BasicBlock *> queue{s};
        while (!queue.empty()) {
            BasicBlock * n = queue.front();
            queue.pop_front();
            if (*n == *parent) {
                return;
            }

            for (auto succ : n->successors) {
                if (succ && succ->depth > block->depth) {
                    queue.push_back(succ);
                }
            }
        }
    }

    if (block->depth <= parent->depth) {
        block->depth = parent->depth + 1;
        for (auto && s : block->successors) {
            if (s) {
                update_depth(s, block);
            }
        }
    }
}

} // namespace

size_t BlockHash::operator()(const BasicBlock * const block) const { return block->m_const_id; }

BasicBlock::BasicBlock(uint32_t const_id, uint32_t id, CFG * const cfg)
    : m_const_id{const_id}, id{id}, depth{0}, instructions{}, predecessors{}, successors{},
      loop_header{false}, p_cfg{cfg} {};

BasicBlock * BasicBlock::get_successor(int index) const {
    assert(index == 0 || index == 1);
    return successors.at(index);
}

BasicBlock * BasicBlock::left_successor() const { return get_successor(0); }

BasicBlock * BasicBlock::right_successor() const { return get_successor(1); }

void BasicBlock::set_successor(BasicBlock * n, int index) {
    assert(index == 0 || index == 1);

    // Do not silently replace a block
    assert(!successors.at(index) || n == nullptr);

    if (n != nullptr) {
        // If the depth of the new successor is less than the depth of the current
        // block increase that depth
        update_depth(n, this);
    }

    successors.at(index) = n;
    p_cfg->sort();
}

void BasicBlock::set_right_successor(BasicBlock * n) {
    set_successor(std::forward<BasicBlock *>(n), 1);
}

void BasicBlock::set_left_successor(BasicBlock * n) {
    set_successor(std::forward<BasicBlock *>(n), 0);
}

std::string BasicBlock::serialize(unsigned indent) const {
    const std::string ind = Private::indenter(indent + 1);

    std::stringstream ss{};
    ss << Private::indenter(indent) << "BasicBlock {\n"
       << ind << "id = { " << id << " }\n"
       << ind << "loop_header = { " << (loop_header ? "true" : "false") << " }\n"
       << ind << "predecessors = {" << ind << "instructions = {";
    for (auto && inst : instructions) {
        ss << "\n" << inst->serialize(indent + 2);
    }
    if (!instructions.empty()) {
        ss << "\n";
    }
    ss << ind << "}";

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
    ss << Private::indenter(indent) << " }";

    return ss.str();
}

bool BasicBlock::operator==(const BasicBlock & other) const { return this->id == other.id; }

bool BasicBlock::operator!=(const BasicBlock & other) const { return this->id != other.id; }

bool BasicBlock::operator<(const BasicBlock & other) const { return depth < other.depth; }

CFG::NodeVec::iterator BasicBlock::begin() {
    // Because the storage is flat, even when we run forward to this index + 1,
    // we can still be returning blocks with the same depth as the start block,
    // which is incorrect.
    auto itr = std::next(p_cfg->begin(), id);
    while ((*itr)->depth <= depth) {
        itr = std::next(itr);
    }
    return itr;
}
CFG::NodeVec::iterator BasicBlock::end() { return p_cfg->end(); }

CFG::NodeVec::const_iterator BasicBlock::cbegin() const {
    auto itr = std::next(p_cfg->cbegin(), id);
    while ((*itr)->depth <= depth) {
        itr = std::next(itr);
    }
    return itr;
}
CFG::NodeVec::const_iterator BasicBlock::cend() const { return p_cfg->cend(); }

CFG::NodeVec::reverse_iterator BasicBlock::rbegin() {
    // we have the distance from the start of the vector, but we need to get the
    // distance from the back
    const uint64_t distance = p_cfg->nodes.size() - id;
    return std::next(p_cfg->rbegin(), distance);
}
CFG::NodeVec::reverse_iterator BasicBlock::rend() { return p_cfg->rend(); }

CFG::NodeVec::const_reverse_iterator BasicBlock::crbegin() const {
    const uint64_t distance = p_cfg->nodes.size() - id;
    return std::next(p_cfg->crbegin(), distance);
}
CFG::NodeVec::const_reverse_iterator BasicBlock::crend() const { return p_cfg->crend(); }

void link_blocks(BasicBlock * pred, BasicBlock * succ, bool right) {
    if (right) {
        pred->set_right_successor(succ);
    } else {
        pred->set_left_successor(succ);
    }
    succ->predecessors.emplace(pred);
}

void reparent(BasicBlock * from, BasicBlock * to) {
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

} // namespace MIR::IR
