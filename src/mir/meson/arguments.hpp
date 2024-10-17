// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

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
 * Specialized include types
 *
 * Used to specify exactly what kind of include this is
 */
enum class IncludeType {
    /// a standard include, such as -Ifoo
    BASE,

    /// A system type include, such as -isystem foo
    SYSTEM,
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
  private:
    std::string _value;
    Type _type;
    IncludeType inc_type;

  public:
    Argument(std::string v, const Type & t);
    Argument(std::string v, const Type & t, const IncludeType & i);

    /// The value of the argument
    std::string value() const;

    /// The type of the argument
    Type type() const;

    /// Include type specialization
    IncludeType include_type() const;

    std::string print() const;

    bool operator==(const Argument &) const;
};

} // namespace MIR::Arguments
