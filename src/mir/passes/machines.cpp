// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include <cassert>

#include "exceptions.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

using namespace MIR::Machines;

std::optional<Machine> machine_map(const std::string & func_name) {
    if (func_name == "build_machine") {
        return MIR::Machines::Machine::BUILD;
    } else if (func_name == "host_machine") {
        return MIR::Machines::Machine::HOST;
    } else if (func_name == "target_machine") {
        return MIR::Machines::Machine::TARGET;
    } else {
        return std::nullopt;
    }
}

MIR::Object lower_function(const std::string & holder, const std::string & name,
                           const Info & info) {
    if (name == "cpu_family") {
        // TODO: it's probably going to be useful to have a helper for this...
        return MIR::Object{std::make_unique<MIR::String>(info.cpu_family)};
    } else if (name == "cpu") {
        // TODO: it's probably going to be useful to have a helper for this...
        return MIR::Object{std::make_unique<MIR::String>(info.cpu)};
    } else if (name == "system") {
        // TODO: it's probably going to be useful to have a helper for this...
        return MIR::Object{std::make_unique<MIR::String>(info.system())};
    } else if (name == "endian") {
        return MIR::Object{std::make_unique<MIR::String>(
            info.endian == MIR::Machines::Endian::LITTLE ? "little" : "big")};
    } else {
        throw Util::Exceptions::MesonException{holder + " has no method " + name};
    }
}

using MachineInfo = MIR::Machines::PerMachine<MIR::Machines::Info>;

std::optional<Object> lower_functions(const MachineInfo & machines, const Object & obj) {
    if (std::holds_alternative<std::unique_ptr<MIR::FunctionCall>>(obj)) {
        const auto & f = std::get<std::unique_ptr<MIR::FunctionCall>>(obj);
        const auto & holder = f->holder.value_or("");

        auto maybe_m = machine_map(holder);
        if (maybe_m.has_value()) {
            const auto & info = machines.get(maybe_m.value());

            return lower_function(holder, f->name, info);
        }
    }
    return std::nullopt;
}

} // namespace

bool machine_lower(BasicBlock * block, const MachineInfo & machines) {
    const auto cb = [&](const Object & o) { return lower_functions(machines, o); };

    return function_walker(block, cb);
};

} // namespace MIR::Passes
