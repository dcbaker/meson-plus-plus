// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <vector>
#include <string>

namespace intermediate {

class IncludeDirectories {
public:
    IncludeDirectories() : dirs() {};
    IncludeDirectories(const std::vector<std::string> d) : dirs{d} {};
    std::vector<std::string> getDirs() { return dirs; };
private:
    const std::vector<std::string> dirs;
};

};