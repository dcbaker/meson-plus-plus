// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

/**
 * Exceptions
 */

#pragma once

#include <stdexcept>
#include <string>

namespace Util::Exceptions {

/**
 * Base Meson++ Exception type
 */
class MesonException : public std::runtime_error {
  public:
    using std::runtime_error::runtime_error;
};

/**
 * Exception for invalid arguments passed to a function
 */
class InvalidArguments : public MesonException {
  public:
    using MesonException::MesonException;
};

} // namespace Util::Exceptions
