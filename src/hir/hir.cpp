// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "hir.hpp"

namespace HIR {

Variable::operator bool() const { return !name.empty(); };

} // namespace HIR
