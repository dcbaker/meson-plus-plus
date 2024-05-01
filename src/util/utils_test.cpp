// SPDX-License-Indentifier: Apache-2.0
// Copyright © 2024 Intel Corporation

#include "utils.hpp"

#include <gtest/gtest.h>

TEST(split, simple) {
    std::vector<std::string> expected{"A", "B", "C"};
    auto && got = Util::split("A B C", " ");
    ASSERT_EQ(expected, got);
}
