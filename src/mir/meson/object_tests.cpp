// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <gtest/gtest.h>

#include "objects.hpp"

using namespace MIR::Objects;

TEST(file, built_relative_to_build) {
    File f{"foo.c", "", true, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_build_dir(), "foo.c");
}

TEST(file, built_relative_to_build_subdir) {
    File f{"foo.c", "sub", true, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_build_dir(), "sub/foo.c");
}

TEST(file, built_relative_to_source) {
    File f{"foo.c", "", true, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_source_dir(), "build/foo.c");
}

TEST(file, built_relative_to_source_subdir) {
    File f{"foo.c", "sub", true, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_source_dir(), "../build/sub/foo.c");
}

TEST(file, static_relative_to_build) {
    File f{"foo.c", "", false, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_build_dir(), "../foo.c");
}

TEST(file, static_relative_to_build_subdir) {
    File f{"foo.c", "sub", false, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_build_dir(), "../../sub/foo.c");
}

TEST(file, static_relative_to_source) {
    File f{"foo.c", "", false, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_source_dir(), "foo.c");
}

TEST(file, static_relative_to_source_subdir) {
    File f{"foo.c", "sub", false, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_source_dir(), "sub/foo.c");
}

TEST(file, equal) {
    File f{"foo.c", "sub", false, "/home/user/src", "/home/user/src/build"};
    File g{"foo.c", "sub", false, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f, g);

    File h{"sub/foo.c", "", false, "/home/user/src", "/home/user/src/build"};
    File i{"foo.c", "sub", false, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(h, i);
}

TEST(file, not_equal) {
    File f{"foo.c", "sub", false, "/home/user/src", "/home/user/src/build"};
    File g{"foo.c", "sub", true, "/home/user/src", "/home/user/src/build"};
    ASSERT_NE(f, g);

    File h{"foo.c", "sub2", true, "/home/user/src", "/home/user/src/build"};
    ASSERT_NE(f, h);

    File i{"foO.c", "sub", true, "/home/user/src", "/home/user/src/build"};
    ASSERT_NE(f, i);
}
