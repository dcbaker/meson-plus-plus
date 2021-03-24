// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <string>
#include <istream>
#include <vector>
#include <memory>

#include "parser.yy.hpp"

namespace Frontend {

class Driver {
  public:
    Driver(){};
    ~Driver(){};

    std::unique_ptr<AST::CodeBlock> parse(std::istream &);
    std::unique_ptr<AST::CodeBlock> parse(const std::string &);

    std::string name;
};

} // namespace Frontend
