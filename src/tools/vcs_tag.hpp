// SPDX-License-Identifier: Apache-2.0
// Copyright © 2024 Intel Corporation

#pragma once

#include <filesystem>

namespace Tools {

/// @brief Generate a file with version substitutions
/// @param infile the template file to read from
/// @param outfile the name of the file to write to
/// @param version  a version to use if we can't get one from VCS
/// @param replacement  the tag to replace
/// @return An integer value with 0 on success
int generate_vcs_tag(const std::filesystem::path & infile, const std::filesystem::path & outfile,
                     std::string_view version, std::string_view replacement);

} // namespace Tools
