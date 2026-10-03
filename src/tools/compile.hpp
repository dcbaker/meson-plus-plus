// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#pragma once

#include <filesystem>

namespace Tools {

/// @brief Compile a meson source file into MIR serialized format
/// @param infile the file to compile
/// @return an int returncode
int compile(const std::filesystem::path & infile);

} // namespace Tools
