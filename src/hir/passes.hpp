// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

/**
 * Lowering passes for HIR
 */

#pragma once

#include "hir.hpp"

namespace HIR::Passes {

/**
 * Prune dead condition branches
 *
 * Once we've been able to lower away conditions in the condition ndoes we want
 * to trim away dead branches and join the ir lists together so we end up with a
 * single flat list of Objects.
 */
bool branch_pruning(IRList *);

} // namespace HIR::Passes
