// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#pragma once

#include <memory>

#include "ir/graph.hpp"
#include "ir/instruction.hpp"

namespace MIR::Builder {

struct BuilderPrivate;

template <typename T, typename... Params> class InstructionBuilder {
  public:
    InstructionBuilder(Params &&... params)
        : p_inst{std::make_shared<T>(std::forward<Params>(params)...)}, p_var{} {};

    // No copying
    InstructionBuilder(const InstructionBuilder &) = delete;
    InstructionBuilder & operator=(const InstructionBuilder &) = delete;

    InstructionBuilder & set_var(std::string name) {
        p_var.m_name = name;
        return *this;
    }

    std::unique_ptr<IR::Instruction> finalize() {
        return std::make_unique<IR::Instruction>(p_inst, std::move(p_var));
    }

  private:
    std::shared_ptr<T> p_inst;
    IR::Variable p_var{};
};

class Builder {
  public:
    Builder();

    template <typename T, typename... Params>
    InstructionBuilder<T, Params...> new_inst(Params &&... params) {
        return InstructionBuilder<T, Params...>{std::forward<Params>(params)...};
    }

    Builder & add_inst(std::unique_ptr<IR::Instruction> && inst);

    std::shared_ptr<IR::Node> finalize();

  private:
    std::shared_ptr<IR::Node> p_root;
};

} // namespace MIR::Builder
