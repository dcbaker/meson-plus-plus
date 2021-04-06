// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation
// Copyright © 2021 Dylan Baker

#include <gtest/gtest.h>

#include "compiler.hpp"

TEST(detect_compilers, g_plus_plus) {
    // Skip if we don't have g++
    if (system("g++") == 127) {
        GTEST_SKIP();
    }
    const auto comp =
        Meson::Toolchain::Compiler::detect_compiler(Meson::Toolchain::Language::CPP, Meson::Machines::Machine::BUILD, {"g++"});
    ASSERT_NE(comp, nullptr);
    ASSERT_EQ(comp->id(), "gcc");
}

TEST(detect_compilers, clang_plus_plus) {
    // Skip if we don't have clang++
    if (system("clang++") == 127) {
        GTEST_SKIP();
    }
    const auto comp =
        Meson::Toolchain::Compiler::detect_compiler(Meson::Toolchain::Language::CPP, Meson::Machines::Machine::BUILD, {"clang++"});
    ASSERT_NE(comp, nullptr);
    ASSERT_EQ(comp->id(), "clang");
}
