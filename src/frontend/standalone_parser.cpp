// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <iostream>

#include "node.hpp"
#include "interpreter.tab.hpp"

extern node::Block * rootBlock;

int main() {
    yyparse();
    std::cout << rootBlock << std::endl;
    return 0;
}
