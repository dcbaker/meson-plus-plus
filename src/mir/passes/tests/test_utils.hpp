// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#pragma once

#include "ast_to_mir.hpp"
#include "driver.hpp"
#include "exceptions.hpp"
#include "lower.hpp"
#include "meson/state/state.hpp"
#include "mir.hpp"

#include <filesystem>
#include <memory>

static const std::filesystem::path src_root = "/home/test user/src/test project/";
static const std::filesystem::path build_root = "/home/test user/src/test project/builddir/";

std::unique_ptr<Frontend::AST::CodeBlock> parse(const std::string & in);

std::shared_ptr<MIR::BasicBlock> lower(const std::string & in);

inline bool is_bb(const MIR::NextType & next) {
    return std::holds_alternative<std::shared_ptr<MIR::BasicBlock>>(next);
}

inline std::shared_ptr<MIR::BasicBlock> get_bb(const MIR::NextType & next) {
    return std::get<std::shared_ptr<MIR::BasicBlock>>(next);
}

inline bool is_con(const MIR::NextType & next) {
    return std::holds_alternative<std::unique_ptr<MIR::Condition>>(next);
}

inline const std::unique_ptr<MIR::Condition> & get_con(const MIR::NextType & next) {
    return std::get<std::unique_ptr<MIR::Condition>>(next);
}

inline bool is_empty(const MIR::NextType & next) {
    return std::holds_alternative<std::monostate>(next);
}

MIR::State::Persistant make_pstate();
