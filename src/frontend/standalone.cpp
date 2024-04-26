// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include "driver.hpp"

int main(int argc, char ** argv) {
    Frontend::Driver drv{};

    auto block = drv.parse(argv[1]);

    std::cout << block->as_string() << std::endl;

    return 0;
}
