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

    /// An Include Directory, further specified by the specialization type
    INCLUDE,

    /// An argument we don't know how to classify, proxy it along
    RAW,
};

/**
 * An abstract argument container
 *
 * This allows us to lower arguments given in compiler specific form (either
 * from a source like pkg-config, or from the build definitions/cli) and keep
 * them in a platform/compiler agnostic form. Then in the backend the compiler
 * can lower these from a agnostic form into a specific form.
 *
 * This contrasts with Meson's approach of using GCC/Unix style arguments
 * internally, and converting between them.
 */
class Argument {
  public:
    Argument(const std::string & v, const Type & t) : value{v}, type{t} {};

    /// The value of the argument
    const std::string value;

    /// The type of the argument
    const Type type;
};

} // namespace MIR::Arguments
