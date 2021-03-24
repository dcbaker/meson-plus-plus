// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <fstream>
#include <iostream>

#include "driver.hpp"
#include "node.hpp"
#include "parser.yy.hpp"
#include "scanner.hpp"

namespace Frontend {

std::unique_ptr<AST::CodeBlock> Driver::parse(const std::string & s){
    name = s;

    std::ifstream stream{s, std::ios_base::in | std::ios_base::binary};

    return parse(stream);
};

std::unique_ptr<AST::CodeBlock> Driver::parse(std::istream & iss) {
    auto block = std::make_unique<Frontend::AST::CodeBlock>();
    auto scanner = std::make_unique<Frontend::Scanner>(&iss, name);
    auto parser = std::make_unique<Frontend::Parser>(*scanner, block);

    parser->parse();

    return block;
};

} // namespace Frontend
