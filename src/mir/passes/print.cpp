// SPDX-license-identifier: Apache-2.0
// Copyright © 2022 Dylan Baker

#include <cstdlib>
#include <fstream>
#include <unordered_set>

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

void printer_impl(const BasicBlock & block, const uint64_t pass,
                  std::unordered_set<uint64_t> & seen, std::ofstream & stream);

void condition_printer(const Condition & cond, const uint64_t pass,
                       std::unordered_set<uint64_t> & seen, std::ofstream & stream) {
    stream << "    "
           << "Condition { condition = " << cond.condition.print()
           << "; if_true = " << cond.if_true->index << "; if_false = " << cond.if_false->index
           << " }"
           << "\n"
           << "  }\n";
    printer_impl(*cond.if_true, pass, seen, stream);
    printer_impl(*cond.if_false, pass, seen, stream);
}

void printer_impl(const BasicBlock & block, const uint64_t pass,
                  std::unordered_set<uint64_t> & seen, std::ofstream & stream) {
    // Only print a given block once
    if (seen.find(block.index) != seen.end()) {
        return;
    }
    seen.emplace(block.index);

    stream << "  BasicBlock " << block.index << " {\n";
    for (auto && i : block.instructions) {
        stream << "    " << i.print() << "\n";
    }
    std::visit(
        [&](auto && n) {
            using T = std::decay_t<decltype(n)>;
            if constexpr (std::is_same_v<T, std::unique_ptr<Condition>>) {
                condition_printer(*n, pass, seen, stream);
            } else if constexpr (std::is_same_v<T, std::shared_ptr<BasicBlock>>) {
                stream << "    next block: " << n->index << "\n";
                printer_impl(*n, pass, seen, stream);
                stream << "  }\n";
            } else {
                static_assert(std::is_same_v<T, std::monostate>);
                stream << "    [[exit]]\n";
            }
        },
        block.next);
}

} // namespace

bool printer(const BasicBlock & block, const uint64_t pass) {
    const char * print = std::getenv("MESONPP_DEBUG_PRINT_MIR");
    if (print != nullptr) { // TODO: [[unlikely]]
        std::unordered_set<uint64_t> seen{};
        std::ofstream out(print, std::ios::app);
        out << "pass " << pass << " {\n";
        printer_impl(block, pass, seen, out);
        out << "}" << std::endl;
    }

    // Always return false, because the print pass doesn't ever make an progress
    // on lowering
    return false;
}

} // namespace MIR::Passes
