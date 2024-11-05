// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

/**
 * Main ninja backend entry point.
 */

#pragma once

#include "meson/state/state.hpp"
#include "mir.hpp"

namespace Backends::Ninja {

/**
 * Generates a ninja file in the build directory
 */
void generate(const MIR::CFGNode &, const MIR::State::Persistant &);

} // namespace Backends::Ninja
