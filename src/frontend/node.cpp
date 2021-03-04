// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <numeric>

#include "node.hpp"

namespace Frontend::AST {

std::string CodeBlock::as_string() const {
    return std::accumulate(
        std::begin(expressions), std::end(expressions), std::string{},
        [](std::string & s, const std::unique_ptr<Expression> & e) {
            return s.empty() ? e->as_string() : s + ", " + e->as_string();
        });
}

}
