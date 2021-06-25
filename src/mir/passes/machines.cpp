// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include "exceptions.hpp"
#include "passes.hpp"

namespace MIR::Passes {

namespace {

using namespace Meson::Machines;

std::optional<Machine> machine_map(const std::string & func_name) {
    if (func_name == "build_machine") {
        return Meson::Machines::Machine::BUILD;
    } else if (func_name == "host_machine") {
        return Meson::Machines::Machine::HOST;
    } else if (func_name == "target_machine") {
        return Meson::Machines::Machine::TARGET;
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
            info.endian == Meson::Machines::Endian::LITTLE ? "little" : "big")};
    } else {
        throw Util::Exceptions::MesonException{holder + " has no method " + name};
    }
}

} // namespace

bool machine_lower(IRList * ir,
                   const Meson::Machines::PerMachine<Meson::Machines::Info> & machines) {
    bool progress = false;

    auto it = ir->instructions.begin();
    while (it != ir->instructions.end()) {
        auto & i = *it;
        if (std::holds_alternative<std::unique_ptr<MIR::FunctionCall>>(i)) {
            const auto & f = std::get<std::unique_ptr<MIR::FunctionCall>>(i);
            const auto & holder = f->holder.value_or("");

            auto maybe_m = machine_map(holder);
            if (!maybe_m.has_value()) {
                ++it;
                continue;
            }
            const auto & info = machines.get(maybe_m.value());

            MIR::Object new_value = lower_function(holder, f->name, info);

            // Remove the current element, then insert the new element in it's place
            it = ir->instructions.erase(it);
            ir->instructions.insert(it, std::move(new_value));
            progress = true;
        }

        // TODO: need to look into arrays
        // TODO: need to look into dictionaries
        // TODO: need to look into conditions.

        ++it;
    }

    return progress;
};

} // namespace MIR::Passes
