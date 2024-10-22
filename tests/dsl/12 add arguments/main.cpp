// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <iostream>

int main() {
    std::cout << "Hello World!" << std::endl;
#ifndef GLOBAL_ARG
#error "Global define not defined"
#endif
#ifndef PROJECT_ARG
#error "Project define not defined"
#endif
    return 0;
}
