// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <gtest/gtest.h>

#include "objects.hpp"

using namespace MIR::Objects;

TEST(file, built_relative_to_build) {
    File f{"foo.c", true, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_build_dir(), "foo.c");
}

TEST(file, built_relative_to_source) {
    File f{"foo.c", true, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_source_dir(), "build/foo.c");
}

TEST(file, static_relative_to_build) {
    File f{"foo.c", false, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_build_dir(), "../foo.c");
}

TEST(file, static_relative_to_source) {
    File f{"foo.c", false, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_source_dir(), "foo.c");
}
