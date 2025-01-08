// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include <cassert>

#include "exceptions.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

using namespace MIR::Machines;

std::optional<Machine> machine_map(const std::string & func_name) {
    if (func_name == "build_machine") {
        return Machine::BUILD;
    }
    if (func_name == "host_machine") {
        return Machine::HOST;
    }
    if (func_name == "target_machine") {
        return Machine::TARGET;
    }
    return std::nullopt;
}

Object lower_function(const std::string & holder, const std::string & name, const Info & info) {
    if (name == "cpu_family") {
        return std::make_shared<MIR::String>(info.cpu_family);
    }
    if (name == "cpu") {
        return std::make_shared<MIR::String>(info.cpu);
    }
    if (name == "system") {
        return std::make_shared<MIR::String>(info.system());
    }
    if (name == "endian") {
        return std::make_shared<MIR::String>(info.endian == Endian::LITTLE ? "little" : "big");
    }
    throw Util::Exceptions::MesonException{holder + " has no method " + name};
}

using MachineInfo = PerMachine<Info>;

} // namespace

std::optional<Object> machine_lower(const Object & obj, const MachineInfo & machines) {
    if (std::holds_alternative<MIR::FunctionCallPtr>(obj)) {
        const auto & f = std::get<MIR::FunctionCallPtr>(obj);
        if (f->holder && std::holds_alternative<IdentifierPtr>(f->holder.value())) {
            const std::string & holder = std::get<IdentifierPtr>(f->holder.value())->value;

            auto maybe_m = machine_map(holder);
            if (maybe_m.has_value()) {
                const auto & info = machines.get(maybe_m.value());

                Object i = lower_function(holder, f->name, info);
                MIR::set_var(obj, i);
                return i;
            }
        }
    }
    return std::nullopt;
}

} // namespace MIR::Passes
