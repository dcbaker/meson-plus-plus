// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#pragma once

#include <string>
#include <tuple>
#include <vector>

#include "common/backend.hpp"
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
          deps{}, order_deps{}, depfile{} {};
    Target(std::vector<std::string> in, const std::string & out, const TargetType & r,
           const MIR::Toolchain::Language & l, const MIR::Machines::Machine & m,
           std::vector<std::string> args)
        : input{std::move(in)}, output{out}, type{r}, lang{l}, machine{m},
          arguments{std::move(args)}, deps{}, order_deps{}, depfile{} {};
    Target(std::vector<std::string> in, const std::string & out, const TargetType & r,
           const MIR::Toolchain::Language & l, const MIR::Machines::Machine & m,
           std::vector<std::string> args, std::vector<std::string> d, std::vector<std::string> o)
        : input{std::move(in)}, output{out}, type{r}, lang{l}, machine{m},
          arguments{std::move(args)}, deps{std::move(d)}, order_deps{std::move(o)}, depfile{} {};
    Target(std::vector<std::string> in, std::vector<std::string> out, const TargetType & r,
           std::vector<std::string> a, std::vector<std::string> d)
        : input{std::move(in)}, output{std::move(out)}, type{r}, lang{}, machine{},
          arguments{std::move(a)}, deps{std::move(d)}, order_deps{}, depfile{} {};
    Target(std::vector<std::string> in, std::vector<std::string> out, const TargetType & r,
           std::vector<std::string> a, std::vector<std::string> d, std::optional<std::string> df)
        : input{std::move(in)}, output{std::move(out)}, type{r}, lang{}, machine{},
          arguments{std::move(a)}, deps{std::move(d)}, order_deps{}, depfile{std::move(df)} {};

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

    /// @brief depfile, only used for custom_target
    std::optional<std::string> depfile;
};

std::tuple<std::vector<Target>, std::vector<Common::Test>>
mir_to_fir(const MIR::CFGNode & block, const MIR::State::Persistant & pstate);

} // namespace Backends::FIR
