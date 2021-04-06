// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "archiver.hpp"
#include "compiler.hpp"
#include "linker.hpp"
#include "toolchain.hpp"

namespace Meson::Toolchain {

Toolchain get_toolchain(const Language & lang, const Machines::Machine & for_machine) {
    // TODO: handle passing in explicit binary name
    auto compiler = Compiler::detect_compiler(lang, for_machine);
    auto archiver = Archiver::detect_archiver(for_machine);
    auto linker = Linker::detect_linker(compiler, for_machine);
    return Toolchain{std::move(compiler), std::move(linker), std::move(archiver)};
};

} // namespace HIR::Toolchain
