// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "compile.hpp"
#include "frontend/driver.hpp"
#include "frontend/node.hpp"
#include "mir/ast_to_mir.hpp"

#include <iostream>

namespace Tools {

int compile(const std::filesystem::path & infile) {
    Frontend::Driver drv{};
    drv.name = infile.string();
    std::unique_ptr<Frontend::AST::CodeBlock> block = drv.parse(infile.string());
    MIR::IR::CFG mir = MIR::ast_to_mir(block);
    std::cout << mir.serialize() << std::endl;
    return 0;
}

} // namespace Tools
