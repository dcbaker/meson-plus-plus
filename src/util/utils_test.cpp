// SPDX-License-Indentifier: Apache-2.0
// Copyright © 2024 Intel Corporation

#include "utils.hpp"

#include <gtest/gtest.h>

TEST(split, simple) {
    std::vector<std::string> expected{"A", "B", "C"};
    auto && got = Util::split("A B C", " ");
    ASSERT_EQ(expected, got);
}

TEST(join, simple) {
    std::string expected{"a;b;c"};
    auto && got = Util::join({"a", "b", "c"}, ";");
    ASSERT_EQ(expected, got);
}

TEST(join, empty) {
    std::string expected{};
    auto && got = Util::join({}, ";");
    ASSERT_EQ(expected, got);
}
