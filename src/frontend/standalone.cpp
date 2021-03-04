// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <memory>
#include <iostream>
#include <fstream>

#include "node.hpp"
#include "scanner.hpp"
#include "parser.yy.hpp"


int main(int argc, char ** argv) {
    std::ifstream stream;
    stream.open(argv[1]);

    auto block = std::make_unique<Frontend::AST::CodeBlock>();
    auto scanner = std::make_unique<Frontend::Scanner>(&stream);
    auto parser = std::make_unique<Frontend::Parser>(*scanner, block);

    parser->parse();

    std::cout << block->as_string() << std::endl;

    return 0;
}
