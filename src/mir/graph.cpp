// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "graph.hpp"

#include <deque>
#include <set>

namespace MIR {

namespace {

uint32_t node_id_base = 0;

using cfg_cb = std::function<void(std::shared_ptr<Node>)>;

/// @brief A Breadth First Search implementation for the CFG
/// @param root The first node to act on
/// @param cb A function to call on each node
void cfg_bfs(std::shared_ptr<Node> root, cfg_cb & cb) {
    std::deque<std::shared_ptr<Node>> deque{};
    deque.push_back(root);

    std::set<uint32_t> processed{};
    processed.emplace(root->id);

    while (!deque.empty()) {
        std::shared_ptr<Node> node = deque.front();
        cb(node);
        deque.pop_front();

        for (auto & s : node->successors) {
            if (processed.find(s->id) != processed.end()) {
                processed.emplace(s->id);
                deque.emplace_back(s);
            }
        }
    }
}

} // namespace

Node::Node(std::shared_ptr<BasicBlock> b) : id{node_id_base++}, block{std::move(b)} {};

CFG::CFG(std::shared_ptr<Node> r) : root{r} {};

void CFG::apply(std::function<void(std::shared_ptr<Node>)> & cb) const { cfg_bfs(root, cb); }

} // namespace MIR
