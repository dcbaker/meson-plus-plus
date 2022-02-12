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

MIR::Object lower_function(const std::string & holder, const std::string & name,
                           const Info & info) {
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

std::optional<Object> lower_functions(const MachineInfo & machines, const Object & obj) {
    if (std::holds_alternative<std::shared_ptr<MIR::FunctionCall>>(obj)) {
        const auto & f = std::get<std::shared_ptr<MIR::FunctionCall>>(obj);
        if (f->holder && std::holds_alternative<std::unique_ptr<Identifier>>(f->holder.value())) {
            const auto & holder = std::get<std::unique_ptr<Identifier>>(f->holder.value())->value;

            auto maybe_m = machine_map(holder);
            if (maybe_m.has_value()) {
                const auto & info = machines.get(maybe_m.value());

                return lower_function(holder, f->name, info);
            }
        }
    }
    return std::nullopt;
}

} // namespace

bool machine_lower(BasicBlock & block, const MachineInfo & machines) {
    const auto cb = [&](const Object & o) { return lower_functions(machines, o); };

    return function_walker(block, cb);
};

} // namespace MIR::Passes
