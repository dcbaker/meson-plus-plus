// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "graph.hpp"

namespace MIR {

Node::Node(std::shared_ptr<BasicBlock> b) : block{std::move(b)} {};

CFG::CFG(std::shared_ptr<Node> r) : root{r} {};

} // namespace MIR
