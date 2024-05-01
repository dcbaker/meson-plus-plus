// SPDX-License-Indentifier: Apache-2.0
// Copyright © 2024 Intel Corporation

#include "common/backend.hpp"

#include <gtest/gtest.h>

using namespace Backends;

TEST(Test_serialization, serialize) {
    Common::Test test{"foo", "/foo", {"a", "/b/c", "d"}, false};
    std::ostringstream stream{};
    test.serialize(stream);
    ASSERT_EQ(stream.str(),
              "BEGIN_TEST\n  name:foo\n  exe:/foo\n  arguments:a|/b/c|d\n  xfail:0\nEND_TEST\n");
}

TEST(Test_serialization, no_arguments) {
    Common::Test test{"foo", "/foo", {}, false};
    std::ostringstream stream{};
    test.serialize(stream);
    ASSERT_EQ(stream.str(),
              "BEGIN_TEST\n  name:foo\n  exe:/foo\n  arguments:\n  xfail:0\nEND_TEST\n");
}

TEST(Test_serialization, deserialize) {
    std::istringstream str{"SERIAL_VERSION:0\nBEGIN_TEST\n  name:foo\n  exe:/foo\n  "
                           "arguments:a|/b/c|d\n  xfail:1\nEND_TEST\n"};
    auto && tests = Common::deserialize_tests(str);
    EXPECT_EQ(tests.size(), 1);

    auto && test = tests.at(0);
    EXPECT_EQ(test.name, "foo");
    EXPECT_EQ(test.should_fail, true);
    EXPECT_EQ(test.arguments, (std::vector<std::string>{"a", "/b/c", "d"}));
    ASSERT_EQ(test.exe, "/foo");
}
