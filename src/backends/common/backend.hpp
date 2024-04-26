// SPDX-License-Indentifier: Apache-2.0
// Copyright © 2024 Intel Corporation

#pragma once

#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

namespace Backends::Common {

namespace fs = std::filesystem;

class Test {
  public:
    Test() = default;
    Test(std::string n, fs::path e) : name{std::move(n)}, exe{std::move(e)} {};
    // Test(const char * serial);

    /// The name of the test
    std::string name;

    /// Path to the executable to be built
    fs::path exe;

    /// @brief Convert the test into a byte stream suitable for serialization
    /// @return a byte stream
    void serialize(std::ostream &) const;
};

void serialize_tests(const std::vector<Test> & tests, const fs::path & p);

std::vector<Test> deserialize_tests(std::istream & in);

std::vector<Test> load_tests(const fs::path & p);

} // namespace Backends::Common
