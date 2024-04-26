// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <gtest/gtest.h>

#include "mir.hpp"

TEST(file, built_relative_to_build) {
    MIR::File f{"foo.c", "", true, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_build_dir(), "foo.c");
}

TEST(file, built_relative_to_build_subdir) {
    MIR::File f{"foo.c", "sub", true, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_build_dir(), "sub/foo.c");
}

TEST(file, built_relative_to_source) {
    MIR::File f{"foo.c", "", true, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_source_dir(), "build/foo.c");
}

TEST(file, built_relative_to_source_subdir) {
    MIR::File f{"foo.c", "sub", true, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_source_dir(), "../build/sub/foo.c");
}

TEST(file, static_relative_to_build) {
    MIR::File f{"foo.c", "", false, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_build_dir(), "../foo.c");
}

TEST(file, static_relative_to_build_subdir) {
    MIR::File f{"foo.c", "sub", false, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_build_dir(), "../../sub/foo.c");
}

TEST(file, static_relative_to_source) {
    MIR::File f{"foo.c", "", false, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_source_dir(), "foo.c");
}

TEST(file, static_relative_to_source_subdir) {
    MIR::File f{"foo.c", "sub", false, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f.relative_to_source_dir(), "sub/foo.c");
}

TEST(file, equal) {
    MIR::File f{"foo.c", "sub", false, "/home/user/src", "/home/user/src/build"};
    MIR::File g{"foo.c", "sub", false, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(f, g);

    MIR::File h{"sub/foo.c", "", false, "/home/user/src", "/home/user/src/build"};
    MIR::File i{"foo.c", "sub", false, "/home/user/src", "/home/user/src/build"};
    ASSERT_EQ(h, i);
}

TEST(file, not_equal) {
    MIR::File f{"foo.c", "sub", false, "/home/user/src", "/home/user/src/build"};
    MIR::File g{"foo.c", "sub", true, "/home/user/src", "/home/user/src/build"};
    ASSERT_NE(f, g);

    MIR::File h{"foo.c", "sub2", true, "/home/user/src", "/home/user/src/build"};
    ASSERT_NE(f, h);

    MIR::File i{"foO.c", "sub", true, "/home/user/src", "/home/user/src/build"};
    ASSERT_NE(f, i);
}

TEST(number, equal) {
    MIR::Number two{2};
    MIR::Number two_2{2};

    ASSERT_EQ(two, two_2);
}

TEST(number, not_equal) {
    MIR::Number one{1};
    MIR::Number two{2};

    ASSERT_NE(two, one);
}
