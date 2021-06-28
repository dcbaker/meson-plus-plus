// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

/* Abstractions for command line arguments.
 */

#pragma once

#include <string>

namespace MIR::Arguments {

/**
 * The kind of argument that this is
 */
enum class Type {
    /// A Pre-processor define (ex, -D...)
    DEFINE,
    /// A library to link with (Ex, -lfoo or /path/to/foo.a)
    LINK,
    /// A path to search for libraries (ex, -L...)
    LINK_SEARCH,
    /// An argument we don't know how to classify, proxy it along
    RAW,
};

/**
 * An abstract argument container
 */
class Argument {
  public:
    Argument(const std::string & v, const Type & t) : value{v}, type{t} {};
    virtual ~Argument(){};

    const std::string value;
    const Type type;
};

} // namespace MIR::Arguments
