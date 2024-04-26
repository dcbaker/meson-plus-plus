// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation
// Copyright © 2021-2024 Intel Corporation

#include <gtest/gtest.h>

#include "compiler.hpp"

TEST(detect_compilers, g_plus_plus) {
    // Skip if we don't have g++
    if (system("g++") == 127) {
        GTEST_SKIP();
    }
    const auto comp = MIR::Toolchain::Compiler::detect_compiler(
        MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD, {"g++"});
    ASSERT_NE(comp, nullptr);
    ASSERT_EQ(comp->id(), "gcc");
}

TEST(detect_compilers, clang_plus_plus) {
    // Skip if we don't have clang++
    if (system("clang++") == 127) {
        GTEST_SKIP();
    }
    const auto comp = MIR::Toolchain::Compiler::detect_compiler(
        MIR::Toolchain::Language::CPP, MIR::Machines::Machine::BUILD, {"clang++"});
    ASSERT_NE(comp, nullptr);
    ASSERT_EQ(comp->id(), "clang");
}
