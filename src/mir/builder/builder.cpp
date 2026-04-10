// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "builder.hpp"
#include "ir.hpp"

#include <cassert>

namespace MIR::Builder {

Builder::Builder() : p_root{std::make_shared<IR::Node>()} {};

std::shared_ptr<IR::Node> Builder::finalize() const { return p_root; }

Builder & Builder::add_inst(std::unique_ptr<IR::Instruction> && inst) {
    assert(inst != nullptr);
    p_root->block->instructions.emplace_back(std::move(inst));
    return *this;
}

} // namespace MIR::Builder
