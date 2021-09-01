// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/**
 * Main ninja backend entry point.
 */

#pragma once

#include "mir/mir.hpp"

namespace Backends::Ninja {

void generate(MIR::BasicBlock *);

}
