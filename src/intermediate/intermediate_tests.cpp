// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "gtest/gtest.h"

#include "include_directories.hpp"

TEST(IncludeDirectories, getDirsIsCopy) {
    std::vector<std::string> v{};
    intermediate::IncludeDirectories inc(v);
    auto got = inc.getDirs();
    ASSERT_EQ(v, got);

    v.push_back("foo");
    ASSERT_NE(v, got);
}