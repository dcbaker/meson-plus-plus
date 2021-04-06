// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation
// Copyright © 2021 Dylan Baker

#include <gtest/gtest.h>

#include "archiver.hpp"

TEST(detect_archivers, gnu) {
    // TODO: there are multiple binaries called ar, how can we be sure we're
    // getting the one we expect?

    // Skip if we don't have g++
    if (system("ar") == 127) {
        GTEST_SKIP();
    }
    const auto comp =
        Meson::Toolchain::Archiver::detect_archiver(Meson::Machines::Machine::BUILD, {"ar"});
    ASSERT_NE(comp, nullptr);
    ASSERT_EQ(comp->id(), "gnu");
}
