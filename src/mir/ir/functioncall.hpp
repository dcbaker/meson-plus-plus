// SPDX-License-Identifier: Apache-2.0
// Copyright © 2026 Intel Corporation

#pragma once

#include "instruction.hpp"

#include <optional>
#include <string>

namespace MIR::IR {

using PositionalArguments = std::vector<InstructionType>;
using KeywordArguments = std::vector<std::pair<InstructionType, InstructionType>>;

std::string serialize(const PositionalArguments & p, unsigned indent = 0);
std::string serialize(const KeywordArguments & k, unsigned indent = 0);

/// Meson allows namespaces such as modules and the `meson` object to be
/// aliased, but it does not allow functions to be aliased. This means that we
/// must store the namespace as an InstructionType, since it could be an
/// identifier or an import() function call

/// @brief A function call
class FunctionCall {
  public:
    FunctionCall(std::string name);
    FunctionCall(std::string name, InstructionType && ns);
    FunctionCall(std::string name, std::string ns);
    FunctionCall(std::string name, PositionalArguments && pos, KeywordArguments && kws);
    FunctionCall(std::string name, InstructionType && ns, PositionalArguments && pos,
                 KeywordArguments && kws);

    /// @brief provide a serialized form of this instruction
    std::string serialize(unsigned indent = 0) const;

    /// @brief The name of the function
    std::string m_name;

    /// @brief The namespace of the function. null means that it doesn't come from a namespace
    std::optional<InstructionType> m_namespace;

    /// @brief The positional arguments of this function
    PositionalArguments m_pos;

    /// @brief The keyword arguments of this function
    KeywordArguments m_kws;
};

} // namespace MIR::IR
