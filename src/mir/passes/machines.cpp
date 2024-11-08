// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

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

Instruction lower_function(const std::string & holder, const std::string & name,
                           const Info & info) {
    if (name == "cpu_family") {
        return MIR::String{info.cpu_family};
    }
    if (name == "cpu") {
        return MIR::String{info.cpu};
    }
    if (name == "system") {
        return MIR::String{info.system()};
    }
    if (name == "endian") {
        return MIR::String{info.endian == Endian::LITTLE ? "little" : "big"};
    }
    throw Util::Exceptions::MesonException{holder + " has no method " + name};
}

using MachineInfo = PerMachine<Info>;

std::optional<Instruction> lower_functions(const MachineInfo & machines, const Instruction & obj) {
    if (std::holds_alternative<MIR::FunctionCall>(*obj.obj_ptr)) {
        const auto & f = std::get<MIR::FunctionCall>(*obj.obj_ptr);
        if (std::holds_alternative<Identifier>(*f.holder.obj_ptr)) {
            const auto & holder = std::get<Identifier>(*f.holder.obj_ptr).value;

            auto maybe_m = machine_map(holder);
            if (maybe_m.has_value()) {
                const auto & info = machines.get(maybe_m.value());

                Instruction i = lower_function(holder, f.name, info);
                i.var = obj.var;
                return i;
            }
        }
    }
    return std::nullopt;
}

} // namespace

bool machine_lower(std::shared_ptr<CFGNode> block, const MachineInfo & machines) {
    const auto cb = [&](const Instruction & o) { return lower_functions(machines, o); };

    return instruction_walker(*block, {cb});
};

} // namespace MIR::Passes
