// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <set>
#include <string>
#include <vector>

namespace MIR::IR {

// I'm concerned about mega headers
class BasicBlock;

/// @brief A single node the Control Flow Graph
class Node {
  public:
    Node();
    Node(std::shared_ptr<BasicBlock> b);
    Node(uint32_t i, std::shared_ptr<BasicBlock> b);
    Node(bool is_header);

    const uint32_t id;

    /// @brief  The block of this node
    std::shared_ptr<BasicBlock> block;

    // A Node has ownership of it's successors, but to break possible reference
    // chains they only have a weak reference to their predecessors

    /// @brief Possible entries to this node.
    std::vector<std::weak_ptr<Node>> predecessors;

    /// @brief The possible exits from this node
    std::array<std::shared_ptr<Node>, 2> successors;

    /// @brief Is this block a loop header block
    bool loop_header;

    std::string serialize() const;

    bool operator==(const Node & other) const;
    bool operator!=(const Node & other) const;

    struct Iterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Node;
        using pointer = Node *;
        using reference = Node &;

        Iterator(pointer ptr);
        reference operator*() const;
        pointer operator->();
        Iterator & operator++();
        Iterator operator++(int);

        friend bool operator==(const Iterator & a, const Iterator & b) {
            return a.deque.front() == b.deque.front();
        }

        friend bool operator!=(const Iterator & a, const Iterator & b) {
            return a.deque.front() != b.deque.front();
        }

      private:
        std::deque<pointer> deque;
        std::set<uint32_t> processed;
    };

    Iterator begin();
    Iterator end();
};

/// @brief Link two nodes together
/// @param pred the Predecessor node
/// @param succ the Successor node
/// @param right if the node is the right leg (default: false)
void link_nodes(std::shared_ptr<Node> pred, std::shared_ptr<Node> succ, bool right = false);

/// @brief The representation of the Control Flow Graph
class CFG {
  public:
    CFG(std::shared_ptr<Node>);

    Node::Iterator begin();
    Node::Iterator end();

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;

    std::shared_ptr<Node> root;
};

} // namespace MIR::IR
