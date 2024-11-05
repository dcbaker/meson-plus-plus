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

std::shared_ptr<MIR::CFGNode> lower(const std::string & in);

MIR::State::Persistant make_pstate();
