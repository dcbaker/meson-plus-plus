// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <unordered_set>

namespace MIR::IR {

// I'm concerned about mega headers
class BasicBlock;

class Node;

/// @brief proxies the id of a precseeding node
struct Predecessor {
    Predecessor(std::shared_ptr<Node> n);

    bool operator==(const Predecessor & other) const;

    std::weak_ptr<Node> m_p;
    uint64_t m_id;
};

struct PredecessorHash {
    size_t operator()(const Predecessor & p) const;
};

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
    std::unordered_set<Predecessor, PredecessorHash> predecessors;

    /// @brief Is this block a loop header block
    bool loop_header;

    std::string serialize(unsigned indent = 0) const;

    bool operator==(const Node & other) const;
    bool operator!=(const Node & other) const;

    /// @brief Get the left successor
    /// @return A shared ptr to the left successor
    std::shared_ptr<Node> left_successor() const;

    /// @brief Get the right successor
    /// @return A shared_ptr to the right successor
    std::shared_ptr<Node> right_successor() const;

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

        friend bool operator==(const Iterator & a, const Iterator & b);
        friend bool operator!=(const Iterator & a, const Iterator & b);

      private:
        /// @brief A queue of items to return
        std::deque<pointer> deque;

        /// @brief A fast set to check what is on the queue
        std::unordered_set<uint32_t> queued;

        /// @brief A set tracking which nodes have been returned
        std::unordered_set<uint32_t> visited;
    };

    Iterator begin();
    Iterator end();

    friend void link_nodes(std::shared_ptr<Node>, std::shared_ptr<Node>, bool right);
    friend void reparent(std::shared_ptr<Node>, std::shared_ptr<Node>);

  private:
    /// @brief Set the left successor
    /// @param n the node to be the successor
    void set_left_successor(std::shared_ptr<Node> n);

    /// @brief Set the right successor
    /// @param n the node to be the successor
    void set_right_successor(std::shared_ptr<Node> n);

    void set_successor(std::shared_ptr<Node> n, int index);

    bool has_successor(int index) const;

    /// @brief The possible exits from this node
    std::array<std::weak_ptr<Node>, 2> successors;

    /// @brief This creates ownership of other nodes
    ///
    /// Notably, this breaks circular ownership when a loop
    /// points back to the header block
    std::array<std::shared_ptr<Node>, 2> children;
};

/// @brief Link two nodes together
/// @param pred the Predecessor node
/// @param succ the Successor node
/// @param right if the node is the right leg (default: false)
void link_nodes(std::shared_ptr<Node> pred, std::shared_ptr<Node> succ, bool right = false);

/// @brief Transfer successors from one node to another
/// @param prev the node to take the successors from
/// @param next the node to give them to
/// This also updates the parents of the moved successor(s)
void reparent(std::shared_ptr<Node> from, std::shared_ptr<Node> to);

/// @brief The representation of the Control Flow Graph
class CFG {
  public:
    CFG(std::shared_ptr<Node>);

    Node::Iterator begin();
    Node::Iterator end();

    /// @brief provide a serialized form of this instruction
    std::string serialize(unsigned indent = 0) const;

    std::shared_ptr<Node> root;
};

} // namespace MIR::IR
