// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#pragma once

#include <array>
#include <cstdint>
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace MIR::IR {

// I'm concerned about mega headers
class BasicBlock;

// Circular definitions...
class Node;
class CFG;

struct NodeHash {
    size_t operator()(const Node & p) const;
    size_t operator()(const Node * p) const;
};

/// @brief A single node the Control Flow Graph
class Node {
  public:
    Node(uint32_t i, std::shared_ptr<BasicBlock> b, CFG * const cfg);

    /// @brief The unique identifier for this block
    const uint32_t id;

    /// @brief The depth of this block in the graph
    uint32_t depth;

    /// @brief  The block of this node
    std::shared_ptr<BasicBlock> block;

    /// @brief Possible entries to this node.
    std::unordered_set<Node *, NodeHash> predecessors;

    /// @brief The possible exits from this node
    std::array<Node *, 2> successors;

    /// @brief Is this block a loop header block
    bool loop_header;

    std::string serialize(unsigned indent = 0) const;

    bool operator==(const Node & other) const;
    bool operator!=(const Node & other) const;

    /// @brief Get the left successor
    /// @return A shared ptr to the left successor
    Node * left_successor() const;

    /// @brief Get the right successor
    /// @return A shared_ptr to the right successor
    Node * right_successor() const;

    /// @brief Set the left successor
    /// @param n the node to be the successor
    void set_left_successor(Node * n);

    /// @brief Set the right successor
    /// @param n the node to be the successor
    void set_right_successor(Node * n);

    void set_successor(Node * n, int index);

    struct Iterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Node;
        using pointer = Node *;
        using reference = Node &;

        Iterator(pointer head);
        reference operator*();
        pointer operator->();
        Iterator & operator++();
        Iterator operator++(int);

        friend bool operator==(const Iterator & a, const Iterator & b);
        friend bool operator!=(const Iterator & a, const Iterator & b);

      private:
        /// @brief The current pointer
        pointer p_current;

        /// @brief The depth of the starting node
        /// We should never visit a node with a higher depth than this
        uint32_t p_depth;

        /// @brief A queue of items to return
        std::map<uint32_t, std::deque<pointer>> p_queue;
    };

    Iterator begin();
    Iterator end();

    struct RIterator {
      public:
        using iterator_category = std::forward_iterator_tag;
        using difference_type = std::ptrdiff_t;
        using value_type = Node;
        using pointer = Node *;
        using reference = Node &;

        RIterator(pointer head);
        reference operator*();
        pointer operator->();
        RIterator & operator++();
        RIterator operator++(int);

        friend bool operator==(const RIterator & a, const RIterator & b);
        friend bool operator!=(const RIterator & a, const RIterator & b);

      private:
        /// @brief The current pointer
        pointer p_current;

        /// @brief The depth of the starting node
        /// We should never visit a node with a higher depth than this
        uint32_t p_depth;

        /// @brief A queue of items to return
        std::map<uint32_t, std::deque<pointer>> p_queue;
    };

    RIterator rbegin();
    RIterator rend();

  private:
    CFG * const p_cfg;
};

/// @brief Link two nodes together
/// @param pred the Predecessor node
/// @param succ the Successor node
/// @param right if the node is the right leg (default: false)
void link_nodes(Node * pred, Node * succ, bool right = false);

/// @brief Transfer successors from one node to another
/// @param prev the node to take the successors from
/// @param next the node to give them to
/// This also updates the parents of the moved successor(s)
void reparent(Node * from, Node * to);

/// @brief The representation of the Control Flow Graph
class CFG {
  public:
    CFG();

    /// @brief Ownership of every node within the graph
    std::unordered_map<uint32_t, std::unique_ptr<Node>> nodes;

    Node * head() const;
    Node * tail() const;
    Node * next();

    /// @brief provide a serialized form of this instruction
    /// TODO: could be const with a const iterator...
    std::string serialize(unsigned indent = 0);

    // Convenience wrapper around Node iterators
    Node::Iterator begin();
    Node::Iterator end();

    Node::RIterator rbegin();
    Node::RIterator rend();

  private:
    /// @brief The counter for the block
    uint32_t p_block_counter;
};

} // namespace MIR::IR
