// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <memory>

#include "archiver.hpp"
#include "common.hpp"
#include "compiler.hpp"
#include "linker.hpp"

namespace HIR::Toolchain {

/**
 * Holds the tool chain for one language, for one machine
 */
class Toolchain {
  public:
    Toolchain(std::unique_ptr<Compiler::Compiler> && c, std::unique_ptr<Linker::Linker> && l)
        : compiler{std::move(c)}, linker{std::move(l)}, archiver{nullptr} {};
    Toolchain(std::unique_ptr<Compiler::Compiler> && c, std::unique_ptr<Linker::Linker> && l,
              std::unique_ptr<Archiver::Archiver> && a)
        : compiler{std::move(c)}, linker{std::move(l)}, archiver{std::move(a)} {};
    ~Toolchain(){};

    const std::unique_ptr<Compiler::Compiler> compiler;
    const std::unique_ptr<Linker::Linker> linker;
    const std::unique_ptr<Archiver::Archiver> archiver;
};

Toolchain get_toolchain(const Language & l);

} // namespace HIR::Toolchain
