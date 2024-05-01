// SPDX-License-Indentifier: Apache-2.0
// Copyright © 2024 Intel Corporation

#include <iostream>
#include <string>

int main(int argc, char * argv[]) {
    if (argc != 2) {
        std::cerr << "Takes exactly 1 argument" << std::endl;
        return 1;
    }

    std::string value = argv[1];
    return std::stoi(value);
}
