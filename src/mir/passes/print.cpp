// SPDX-License-Identifier: Apache-2.0
// Copyright © 2022-2025 Intel Corporation

#include <cstdlib>
#include <fstream>
#include <unordered_set>

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

Printer::Printer(uint32_t p) : pass{p} {
    const char * print = std::getenv("MESONPP_DEBUG_PRINT_MIR");
    if (print != nullptr) { // TODO: [[unlikely]]
        out.open(print, std::ios::app);
        increment();
    }
}

Printer::~Printer() {
    if (out.is_open()) {
        out << "}" << std::endl;
        out.close();
    }
}

void Printer::increment() {
    if (out.is_open()) {
        if (pass != 0) {
            out << "}\n";
        }
        out << "pass " << ++pass << " {" << std::endl;
    }
}

bool Printer::operator()(const std::shared_ptr<CFGNode> block) {
    if (out.is_open()) {
        // Only print a given block once
        out << "  CFGNode " << block->index << " {\n";
        for (auto && i : block->block->instructions) {
            out << "    " << std::visit([](const auto & o) { return o->print(); }, i) << "\n";
        }
        if (block->successors.empty()) {
            out << "    [[exit]]\n";
        }
        out << "  }\n";
    }

    // Always return false, because the print pass doesn't ever make an progress
    // on lowering
    return false;
}

} // namespace MIR::Passes
