// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <fstream>
#include <iostream>
#include <memory>

#include "node.hpp"
#include "parser.yy.hpp"
#include "scanner.hpp"

int main(int argc, char ** argv) {
    std::ifstream stream{argv[1], std::ios_base::in | std::ios_base::binary};

    auto block = std::make_unique<Frontend::AST::CodeBlock>();
    auto scanner = std::make_unique<Frontend::Scanner>(&stream);
    auto parser = std::make_unique<Frontend::Parser>(*scanner, block);

    parser->parse();

    std::cout << block->as_string() << std::endl;

    return 0;
}
