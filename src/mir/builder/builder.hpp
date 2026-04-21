// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#pragma once

#include <list>
#include <memory>

#include "ir/graph.hpp"
#include "ir/instruction.hpp"

namespace MIR::builder {

namespace {

template <typename, typename> constexpr bool is_one_of_variants_types = false;

template <typename... Ts, typename T>
constexpr bool is_one_of_variants_types<std::variant<Ts...>, T> = (std::is_same_v<T, Ts> || ...);

template <typename T>
constexpr bool is_mir_instruction = is_one_of_variants_types<MIR::IR::InstructionType, T>;

} // namespace

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

    template <int..., typename U = T,
              typename = std::enable_if_t<std::is_same_v<U, MIR::IR::FunctionCall>>>
    InstructionBuilder & add_pos_arg(IR::InstructionType && arg) {
        p_inst->m_pos.emplace_back(std::move(arg));
        return *this;
    }

    template <int..., typename U = T,
              typename = std::enable_if_t<std::is_same_v<U, MIR::IR::FunctionCall>>>
    InstructionBuilder & add_kw_arg(IR::InstructionType && key, IR::InstructionType && value) {
        p_inst->m_kws.emplace_back(std::move(key), std::move(value));
        return *this;
    }

    template <int..., typename U = T,
              typename = std::enable_if_t<std::is_same_v<U, MIR::IR::Array>>>
    InstructionBuilder & append(IR::InstructionType && arg) {
        p_inst->m_value.emplace_back(std::move(arg));
        return *this;
    }

    template <int..., typename U = T, typename = std::enable_if_t<std::is_same_v<U, MIR::IR::Dict>>>
    InstructionBuilder & append(IR::InstructionType && key, IR::InstructionType && value) {
        p_inst->m_value.emplace(std::move(key), std::move(value));
        return *this;
    }

    std::unique_ptr<IR::Instruction> as_instr() {
        return std::make_unique<IR::Instruction>(p_inst, std::move(p_var));
    }

    operator std::unique_ptr<IR::Instruction>() {
        return std::make_unique<IR::Instruction>(p_inst, std::move(p_var));
    }

    IR::InstructionType as_type() const { return p_inst; }

    operator IR::InstructionType() const { return p_inst; }

  private:
    std::shared_ptr<T> p_inst;
    IR::Variable p_var{};
};

template <typename T, typename = std::enable_if<is_mir_instruction<T>>, typename... Params>
InstructionBuilder<T, Params...> make_instruction(Params &&... params) {
    return InstructionBuilder<T, Params...>{std::forward<Params>(params)...};
}

class Builder {
  public:
    Builder(IR::CFG * cfg);
    Builder(IR::CFG * cfg, IR::Node * node);

    Builder & add_inst(std::unique_ptr<IR::Instruction> && inst);

    Builder & add_condition(std::unique_ptr<IR::Instruction> && inst);

    Builder & set_loop_header(bool v = true);

    Builder left_successor();
    Builder right_successor();

    Builder & link_left_successor(Builder &);
    Builder & link_left_successor(std::shared_ptr<Builder> &);
    Builder & link_left_successor(IR::Node *);

    Builder & link_right_successor(Builder &);
    Builder & link_right_successor(std::shared_ptr<Builder> &);
    Builder & link_right_successor(IR::Node *);

    [[nodiscard]] IR::Node * get() const;

    Builder & set_cursor_begin();
    Builder & set_cursor_end();

  private:
    IR::CFG * p_cfg;

    IR::Node * p_node;

    // Iterator pointing into the instructions as to where to add
    // the next instruction
    std::list<std::unique_ptr<IR::Instruction>>::iterator p_cursor;
};

} // namespace MIR::builder
