// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/**
 * Exceptions
 */

#pragma once

#include <string>

namespace Util::Exceptions {

/**
 * Base Meson++ Exception type
 */
class MesonException {
  public:
    MesonException(const std::string & msg) : message{msg} {};
    virtual ~MesonException(){};

    const std::string message;
};

/**
 * Exception for invalid arguments passed to a function
 */
class InvalidArguments : public MesonException {
  public:
    InvalidArguments(const std::string & msg) : MesonException{msg} {};
    virtual ~InvalidArguments(){};

};

} // namespace Util::Exceptions
