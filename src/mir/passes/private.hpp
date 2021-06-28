// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

/**
 * Implementation details of the MIR passes
 */

#pragma once

#include "mir.hpp"
#include <functional>
#include <optional>

namespace MIR::Passes {

using Callback = std::function<std::optional<Object>(const Object &)>;

/**
 * WAlks each instruction in a basic block, calling the callback on them
 *
 * If the callback returns a value (not std::nullopt), then it will replace the
 * current instruction with that instruction, otherwie it does nothing.
 *
 * Returns true if any changes were made to the block.
 */
bool instruction_walker(BasicBlock *, const Callback &);

} // namespace MIR::Passes
