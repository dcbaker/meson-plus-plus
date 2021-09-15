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

}
