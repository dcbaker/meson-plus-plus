// SPDX-license-identifier: Apache-2.0
// Copyright © 2021-2022 Dylan Baker

#include "state.hpp"

namespace MIR::State {

Dependency::Dependency(std::string ver, std::vector<Arguments::Argument> compile_args,
                       std::vector<Arguments::Argument> link_args)
    : version{std::move(ver)}, compile{std::move(compile_args)}, link{std::move(link_args)},
      found{true} {};
Dependency::Dependency() : version{"undefined"}, found{false} {};

Persistant::Persistant(std::filesystem::path sr_, std::filesystem::path br_)
    : machines{Machines::detect_build()}, source_root{std::move(sr_)}, build_root{
                                                                           std::move(br_)} {};

} // namespace MIR::State
