// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include "driver.hpp"
#include "deserialize.yy.hpp"
#include "scanner.hpp"

#include <fstream>
#include <iostream>
#include <memory>

namespace MIR::IR::Serial {

CFG Driver::parse(const std::string & s) {
    name = s;

    std::ifstream stream{s, std::ios_base::in | std::ios_base::binary};

    return parse(stream);
};

CFG Driver::parse(std::istream & iss) {
    auto node = std::make_shared<Node>();
    auto scanner = std::make_unique<Scanner>(&iss, name);
    auto parser = std::make_unique<Parser>(*scanner, node);

    int res = parser->parse();
    if (res != 0) {
        throw std::exception{};
    }

    return node;
};

} // namespace MIR::IR::Serial
