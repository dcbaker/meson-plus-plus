// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#pragma once

#include <memory>

#include "ir/instruction.hpp"

namespace MIR::Builder {

struct BuilderPrivate;

class Builder {
  private:
    std::unique_ptr<BuilderPrivate> priv;

  public:
    Builder();
};

} // namespace MIR::Builder
