// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/**
 * Provides the main lowering loop implementation
 *
 */

#pragma once

#include "passes.hpp"
#include "state/state.hpp"

namespace MIR {

void lower(BasicBlock *, State::Persistant &);

namespace Passes {

/**
 * Handle the requirements placed on the project() call
 *
 * Such as: it *must* be the first non-comment, no-whitespace code in the root
 * meson.build file. later on, when we handle project() it will simply be an
 * error to have it, so right now we wnat to read it, and delete it.
 */
void lower_project(BasicBlock * block, State::Persistant & pstate);

} // namespace Passes

} // namespace MIR
