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
    Target(std::vector<std::string> in, std::vector<std::string> out, const TargetType & r,
           const MIR::Toolchain::Language & l, const MIR::Machines::Machine & m)
        : input{std::move(in)}, output{std::move(out)}, type{r}, lang{l}, machine{m}, arguments{},
          deps{}, order_deps{} {};
    Target(std::vector<std::string> in, const std::string & out, const TargetType & r,
           const MIR::Toolchain::Language & l, const MIR::Machines::Machine & m,
           std::vector<std::string> args)
        : input{std::move(in)}, output{out}, type{r}, lang{l}, machine{m},
          arguments{std::move(args)}, deps{}, order_deps{} {};
    Target(std::vector<std::string> in, const std::string & out, const TargetType & r,
           const MIR::Toolchain::Language & l, const MIR::Machines::Machine & m,
           std::vector<std::string> args, std::vector<std::string> d, std::vector<std::string> o)
        : input{std::move(in)}, output{out}, type{r}, lang{l}, machine{m},
          arguments{std::move(args)}, deps{std::move(d)}, order_deps{std::move(o)} {};
    Target(std::vector<std::string> in, std::vector<std::string> out, const TargetType & r,
           std::vector<std::string> a)
        : input{std::move(in)}, output{std::move(out)}, type{r}, lang{}, machine{},
          arguments{std::move(a)}, deps{}, order_deps{} {};

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
