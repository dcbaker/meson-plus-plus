// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#pragma once

#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace MIR {

// I'm concerned about mega headers
class BasicBlock;

/// @brief A single node the Control Flow Graph
class Node {
  public:
    Node(std::shared_ptr<BasicBlock> b);

    const uint32_t id;

    /// @brief  The block of this node
    std::shared_ptr<BasicBlock> block;

    /// @brief Possible entries to this node.
    std::vector<std::shared_ptr<Node>> predecessors;

    /// @brief The possible exits from this node
    std::vector<std::shared_ptr<Node>> successors;
};

/// @brief The representation of the Control Flow Graph
class CFG {
  public:
    CFG(std::shared_ptr<Node> r);

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;

    /// @brief Walk the graph calling a callback on each node
    /// @param cb The callback to call
    void apply(std::function<void(std::shared_ptr<Node>)> & cb) const;

    std::shared_ptr<Node> root;
};

} // namespace MIR
