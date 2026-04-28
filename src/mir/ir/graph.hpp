// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <list>
#include <memory>
#include <string>
#include <unordered_set>

#include "instruction.hpp"

namespace MIR::IR {

// Circular definitions...
class BasicBlock;

/// @brief The representation of the Control Flow Graph
class CFG {
  public:
    using NodeVec = std::vector<std::unique_ptr<BasicBlock>>;

    CFG();

    /// @brief Ownership of every node within the graph
    NodeVec nodes;

    BasicBlock * head() const;
    BasicBlock * next();

    void sort();

    /// @brief provide a serialized form of this instruction
    /// TODO: could be const with a const iterator...
    std::string serialize(unsigned indent = 0);

    // Convenience wrapper around Node iterators
    NodeVec::iterator begin();
    NodeVec::iterator end();
    NodeVec::const_iterator cbegin() const;
    NodeVec::const_iterator cend() const;
    NodeVec::reverse_iterator rbegin();
    NodeVec::reverse_iterator rend();
    NodeVec::const_reverse_iterator crbegin() const;
    NodeVec::const_reverse_iterator crend() const;

  private:
    uint32_t p_const_ids;
    uint32_t p_next_const_id();
};

struct BlockHash {
    size_t operator()(const BasicBlock * const node) const;
};

/// @brief A single node the Control Flow Graph
class BasicBlock {
  public:
    using PredecessorType = std::unordered_set<BasicBlock *, BlockHash>;

    BasicBlock(uint32_t const_id, uint32_t id, CFG * const cfg);

    // Nodes cannot be copied
    BasicBlock(const BasicBlock &) = delete;
    BasicBlock & operator=(const BasicBlock &) = delete;

    // It might be possible to implement a move operator, but I don't have a use
    // for one ATM, so just deleting explicitly
    BasicBlock(BasicBlock && node) = delete;
    BasicBlock & operator=(BasicBlock && node) = delete;

    /// @brief The unique identifier for this block
    const uint32_t m_const_id;

    /// @brief The index of this block in the CFG
    uint32_t id;

    /// @brief The depth of this block in the graph
    uint32_t depth;

    /// @brief The list of instructions in this node
    // TODO: There are reasons why this is a unique ptr, explain them
    std::list<std::unique_ptr<IR::Instruction>> instructions;

    /// @brief Possible entries to this node.
    PredecessorType predecessors;

    /// @brief The possible exits from this node
    std::array<BasicBlock *, 2> successors;

    /// @brief Is this block a loop header block
    bool loop_header;

    std::string serialize(unsigned indent = 0) const;

    bool operator==(const BasicBlock & other) const;
    bool operator!=(const BasicBlock & other) const;
    bool operator<(const BasicBlock & other) const;

    /// @brief Get the left successor
    /// @return A shared ptr to the left successor
    BasicBlock * left_successor() const;

    /// @brief Get the right successor
    /// @return A shared_ptr to the right successor
    BasicBlock * right_successor() const;

    /// @brief Set the left successor
    /// @param n the node to be the successor
    void set_left_successor(BasicBlock * n);

    /// @brief Set the right successor
    /// @param n the node to be the successor
    void set_right_successor(BasicBlock * n);

    CFG::NodeVec::iterator begin();
    CFG::NodeVec::iterator end();
    CFG::NodeVec::const_iterator cbegin() const;
    CFG::NodeVec::const_iterator cend() const;
    CFG::NodeVec::reverse_iterator rbegin();
    CFG::NodeVec::reverse_iterator rend();
    CFG::NodeVec::const_reverse_iterator crbegin() const;
    CFG::NodeVec::const_reverse_iterator crend() const;

  private:
    /// @brief Pointer to the CFG that owns this Node
    CFG * p_cfg;

    void set_successor(BasicBlock * n, int index);
    BasicBlock * get_successor(int index) const;
};

/// @brief Link two nodes together
/// @param pred the Predecessor block
/// @param succ the Successor block
/// @param right if the node is the right leg (default: false)
void link_blocks(BasicBlock * pred, BasicBlock * succ, bool right = false);

/// @brief Transfer successors from one node to another
/// @param prev the node to take the successors from
/// @param next the node to give them to
/// This also updates the parents of the moved successor(s)
void reparent(BasicBlock * from, BasicBlock * to);

} // namespace MIR::IR
