// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include "node.hpp"

namespace Frontend::AST {

struct AsStringVisitor {
    std::string operator()(const std::unique_ptr<String> &);
    std::string operator()(const std::unique_ptr<Number> &);
    std::string operator()(const std::unique_ptr<Identifier> &);
    std::string operator()(const std::unique_ptr<Boolean> &);
    std::string operator()(const std::unique_ptr<AdditiveExpression> &);
    std::string operator()(const std::unique_ptr<MultiplicativeExpression> &);
    std::string operator()(const std::unique_ptr<UnaryExpression> &);
};

} // namespace Frontend::AST
