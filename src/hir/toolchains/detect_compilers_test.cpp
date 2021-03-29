// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation
// Copyright © 2021 Dylan Baker

#include <gtest/gtest.h>

#include "compiler.hpp"

TEST(detect_compilers, gpp) {
    // Skip if we don't have g++
    if (system("g++") != 0) {
        GTEST_SKIP();
    }
    const auto comp =
        HIR::Toolchain::Compiler::detect_compiler(HIR::Toolchain::Language::CPP, HIR::Machines::Machine::BUILD);
    ASSERT_EQ(comp->id(), "gcc");
}
