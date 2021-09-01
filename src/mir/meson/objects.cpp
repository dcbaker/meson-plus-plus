// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "objects.hpp"

namespace MIR::Objects {

const bool File::is_built() const { return built; }
const std::string File::get_name() const { return name; }

} // namespace MIR::Objects
