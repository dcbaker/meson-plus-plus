// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#pragma once

#include <string>

namespace MIR::Version {

/// What kind of operation is being done
enum class Operator {
    LT,
    LE,
    NE,
    EQ,
    GE,
    GT,
};

std::string to_string(const Operator & op);

bool compare(const std::string & v1, const Operator & op, const std::string & v2);

} // namespace MIR::Version
