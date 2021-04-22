// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/**
 * Linker detection functions
 */

#include <cassert>
#include <memory>
#include <string>
#include <vector>

#include "compiler.hpp"
#include "linker.hpp"
#include "process.hpp"

namespace Meson::Toolchain::Linker {

namespace {

/**
 * Specialization for GCC (and G++, etc)
 */
std::unique_ptr<Linker> detect_linker_gcc(const std::unique_ptr<Compiler::Compiler> & comp,
                                          const Machines::Machine & machine) {
    auto command = comp->command;
    command.emplace_back("-Wl,--version");
    auto const & [ret, out, err] = Util::process(command);
    // TODO: something smarter her
    if (ret != 0) {
        throw std::exception{};
    }

    if (out.find("GNU ld") != std::string::npos) {
        auto linker = GnuBFD{command};
        return std::make_unique<Drivers::Gnu>(linker);
    }
    assert(false);
};

} // namespace

std::unique_ptr<Linker> detect_linker(const std::unique_ptr<Compiler::Compiler> & comp,
                                      const Machines::Machine & machine) {
    if (comp->id() == "gcc") {
        return detect_linker_gcc(comp, machine);
    }
    assert(false);
};

} // namespace Meson::Toolchain::Linker
