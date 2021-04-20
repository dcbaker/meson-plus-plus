// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "ast_to_ir.hpp"

namespace IR {

namespace {

struct FundementalTypeLowering {
    Holders operator()(const std::unique_ptr<const Frontend::AST::Assignment> & block) {
        ;
    }
};

} // namespace

HolderList lower_ast(const Frontend::AST::CodeBlock &) {
    HolderList bl{};

    return bl;
}

} // namespace IR
