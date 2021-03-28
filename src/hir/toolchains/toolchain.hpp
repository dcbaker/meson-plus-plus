// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <optional>

#include "archiver.hpp"
#include "compiler.hpp"
#include "linker.hpp"

namespace HIR::Toolchain {

/**
 * Holds the tool chain for one language, for one machine
 */
class Toolchain {
  public:
    Toolchain(const Compiler::Compiler & c, const Linker::Linker & l)
        : compiler{c}, linker{l}, archiver{std::nullopt} {};
    Toolchain(const Compiler::Compiler & c, const Linker::Linker & l, const Archiver::Archiver & a)
        : compiler{c}, linker{l}, archiver{a} {};
    ~Toolchain(){};

    const Compiler::Compiler compiler;
    const Linker::Linker linker;
    const std::optional<Archiver::Archiver> archiver;
};

} // namespace HIR::Toolchain
