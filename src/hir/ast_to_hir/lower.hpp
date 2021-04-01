// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#pragma once

#include "node.hpp"
#include "state/state.hpp"

namespace HIR {

/**
 * Convert the AST into HIR
 */
void lower_ast(const Frontend::AST::CodeBlock &, State::Persistant &, State::Transitive &);

}
