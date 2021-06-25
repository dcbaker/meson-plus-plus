// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "mir.hpp"

namespace MIR {

Variable::operator bool() const { return !name.empty(); };

} // namespace MIR
