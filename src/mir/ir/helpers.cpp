// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#include "helpers.hpp"

#include <sstream>

namespace MIR::IR::Private {

std::string indenter(unsigned index) {
    std::stringstream ss{};
    for (unsigned i = 0; i < index; ++i) {
        ss << "  ";
    }
    return ss.str();
}

} // namespace MIR::IR::Private
