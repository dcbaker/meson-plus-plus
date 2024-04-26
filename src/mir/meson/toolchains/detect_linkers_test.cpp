// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation
// Copyright © 2021-2024 Intel Corporation

#include <gtest/gtest.h>

#include "compiler.hpp"
#include "linker.hpp"

TEST(g_plus_plus, bfd) {
    // Skip if we don't have g++ or ld.bfd
    if (system("g++") == 127 || system("ld.bfd") == 127) {
        GTEST_SKIP();
    }
    const auto comp = MIR::Toolchain::Compiler::detect_compiler(
        MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD, {"g++"});
    ASSERT_NE(comp, nullptr);

    const auto link = MIR::Toolchain::Linker::detect_linker(comp, MIR::Machines::Machine::BUILD);
    ASSERT_NE(link, nullptr);
    ASSERT_EQ(link->id(), "ld.bfd");
}
