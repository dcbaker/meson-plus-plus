// SPDX-License-Indentifier: Apache-2.0
// Copyright © 2024 Intel Corporation

#pragma once

#include "backends/common/backend.hpp"
#include "util/process.hpp"

#include <filesystem>

namespace Test {

int run_tests(const std::vector<Backends::Common::Test> & tests,
              const std::filesystem::path & builddir);

} // namespace Test
