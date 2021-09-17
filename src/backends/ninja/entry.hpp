// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

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
void generate(const MIR::BasicBlock * const, const MIR::State::Persistant &);

} // namespace Backends::Ninja
