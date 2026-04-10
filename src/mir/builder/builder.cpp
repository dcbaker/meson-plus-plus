// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "builder.hpp"

#include "ir/graph.hpp"
#include "ir/instructions.hpp"

namespace MIR::Builder {

struct BuilderPrivate {
    BuilderPrivate() : root{std::make_shared<IR::Node>()} {};

    std::shared_ptr<IR::Node> root;
};

Builder::Builder() : priv{std::make_unique<BuilderPrivate>()} {};

} // namespace MIR::Builder
