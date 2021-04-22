// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <optional>

#include "node.hpp"

namespace Frontend::AST {

/**
 * Convert all `subdir()` calls into AST and insert it into the tree.
 */
struct SubdirVisitor {
    std::optional<std::unique_ptr<CodeBlock>> operator()(const std::unique_ptr<Statement> &) const;
    std::optional<std::unique_ptr<CodeBlock>>
    operator()(const std::unique_ptr<IfStatement> &) const;
    std::optional<std::unique_ptr<CodeBlock>>
    operator()(const std::unique_ptr<ForeachStatement> &) const {
        return std::nullopt;
    };
    std::optional<std::unique_ptr<CodeBlock>>
    operator()(const std::unique_ptr<Assignment> &) const {
        return std::nullopt;
    };
    std::optional<std::unique_ptr<CodeBlock>> operator()(const std::unique_ptr<Break> &) const {
        return std::nullopt;
    };
    std::optional<std::unique_ptr<CodeBlock>> operator()(const std::unique_ptr<Continue> &) const {
        return std::nullopt;
    };
};

} // namespace Frontend::AST
