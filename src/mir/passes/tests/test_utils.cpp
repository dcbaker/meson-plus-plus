// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "test_utils.hpp"

std::unique_ptr<Frontend::AST::CodeBlock> parse(const std::string & in) {
    Frontend::Driver drv{};
    std::istringstream stream{in};
    drv.name = src_root / "meson.build";
    return drv.parse(stream);
}

MIR::BasicBlock lower(const std::string & in) {
    auto block = parse(in);
    const MIR::State::Persistant pstate{src_root, build_root};
    return MIR::lower_ast(block, pstate);
}
