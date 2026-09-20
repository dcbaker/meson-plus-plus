// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#pragma once

#include <string>

namespace MIR::IR {

/// @brief A string object
class File {
  public:
    File(std::string_view s);

    /// @brief provide a serialized form of this instruction
    std::string serialize(unsigned indent = 0) const;

    std::string path;
};

} // namespace MIR::IR
