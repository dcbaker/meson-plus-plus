// SPDX-License-Identifier: Apache-2.0
// Copyright © 2022-2024 Intel Corporation

#include <cstdlib>
#include <fstream>
#include <unordered_set>

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

void condition_printer(const Condition & cond, std::ofstream & stream) {
    stream << "    "
           << "Condition { condition = " << cond.condition.print()
           << "; if_true = " << cond.if_true->index << "; if_false = " << cond.if_false->index
           << " }"
           << "\n"
           << "  }\n";
}

void printer_impl(const BasicBlock & block, std::ofstream & stream) {
    // Only print a given block once
    stream << "  BasicBlock " << block.index << " {\n";
    for (auto && i : block.instructions) {
        stream << "    " << i.print() << "\n";
    }
    std::visit(
        [&](auto && n) {
            using T = std::decay_t<decltype(n)>;
            if constexpr (std::is_same_v<T, std::unique_ptr<Condition>>) {
                condition_printer(*n, stream);
            } else if constexpr (std::is_same_v<T, std::shared_ptr<BasicBlock>>) {
                stream << "    next block: " << n->index << "\n"
                       << "  }\n";
            } else {
                static_assert(std::is_same_v<T, std::monostate>);
                stream << "    [[exit]]\n";
            }
        },
        block.next);
}

} // namespace

Printer::Printer(uint32_t p) : pass{p} {
    const char * print = std::getenv("MESONPP_DEBUG_PRINT_MIR");
    if (print != nullptr) { // TODO: [[unlikely]]
        out.open(print, std::ios::app);
        increment();
    }
}

Printer::~Printer() {
    if (out.is_open()) {
        out << "  }\n"
            << "}" << std::endl;
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

bool Printer::operator()(const BasicBlock & block) {
    if (out.is_open()) {
        printer_impl(block, out);
    }

    // Always return false, because the print pass doesn't ever make an progress
    // on lowering
    return false;
}

} // namespace MIR::Passes
