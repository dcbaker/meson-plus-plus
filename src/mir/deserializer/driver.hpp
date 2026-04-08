// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#pragma once

#include "../ir.hpp"

#include <istream>
#include <memory>
#include <string>
#include <vector>

namespace MIR::IR::Serial {

class Driver {
  public:
    Driver();

    CFG parse(std::istream &);
    CFG parse(const std::string &);

    std::string name;
};

} // namespace MIR::IR::Serial
