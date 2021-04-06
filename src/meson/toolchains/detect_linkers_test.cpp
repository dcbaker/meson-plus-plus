// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation
// Copyright © 2021 Dylan Baker

#include <gtest/gtest.h>

#include "compiler.hpp"
#include "linker.hpp"

TEST(g_plus_plus, bfd) {
    // Skip if we don't have g++ or ld.bfd
    if (system("g++") == 127 || system("ld.bfd") == 127) {
        GTEST_SKIP();
    }
    const auto comp =
        Meson::Toolchain::Compiler::detect_compiler(Meson::Toolchain::Language::CPP, Meson::Machines::Machine::BUILD, {"g++"});
    ASSERT_NE(comp, nullptr);

    const auto link = Meson::Toolchain::Linker::detect_linker(comp, Meson::Machines::Machine::BUILD);
    ASSERT_NE(link, nullptr);
    ASSERT_EQ(link->id(), "ld.bfd");
}
