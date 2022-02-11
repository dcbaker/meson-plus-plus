// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <string>
#include <variant>
#include <vector>

#include "meson/machines.hpp"
#include "meson/state/state.hpp"
#include "mir.hpp"

namespace Backends::FIR {

enum class TargetType {
    COMPILE,
    ARCHIVE,
    LINK,
    CUSTOM,
};

/**
 * A Ninja rule to be generated later
 */
class Target {
  public:
    Target(const std::vector<std::string> & in, const std::vector<std::string> & out,
           const TargetType & r, const MIR::Toolchain::Language & l,
           const MIR::Machines::Machine & m)
        : input{in}, output{out}, type{r}, lang{l}, machine{m}, arguments{}, deps{},
          order_deps{} {};
    Target(const std::vector<std::string> & in, const std::string & out, const TargetType & r,
           const MIR::Toolchain::Language & l, const MIR::Machines::Machine & m,
           const std::vector<std::string> & args)
        : input{in}, output{out}, type{r}, lang{l}, machine{m}, arguments{args}, deps{},
          order_deps{} {};
    Target(const std::vector<std::string> & in, const std::string & out, const TargetType & r,
           const MIR::Toolchain::Language & l, const MIR::Machines::Machine & m,
           const std::vector<std::string> & args, const std::vector<std::string> & d,
           const std::vector<std::string> & o)
        : input{in}, output{out}, type{r}, lang{l}, machine{m}, arguments{args}, deps{d},
          order_deps{o} {};
    Target(const std::vector<std::string> & in, const std::vector<std::string> & out,
           const TargetType & r, const std::vector<std::string> & a)
        : input{in}, output{out}, type{r}, lang{}, machine{}, arguments{a}, deps{}, order_deps{} {};

    /// The input for this rule
    const std::vector<std::string> input;

    /// The output of this rule
    const std::vector<std::string> output;

    /// The type of rule this is
    const TargetType type;

    /// The language of this rule
    const MIR::Toolchain::Language lang;

    /// The machine of this rule
    const MIR::Machines::Machine machine;

    /// The arguments for this rule
    const std::vector<std::string> arguments;

    /// Order only inputs
    const std::vector<std::string> deps;

    /// Order only inputs
    const std::vector<std::string> order_deps;
};

std::vector<Target> mir_to_fir(const MIR::BasicBlock & block,
                               const MIR::State::Persistant & pstate);

} // namespace Backends::FIR
