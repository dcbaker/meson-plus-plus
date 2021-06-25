// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include "passes.hpp"

namespace MIR::Passes {

bool branch_pruning(IRList * ir) {
    if (!ir->condition.has_value()) {
        return false;
    }

    const auto & con = ir->condition.value();
    if (!std::holds_alternative<std::unique_ptr<Boolean>>(con.condition)) {
        return false;
    }

    const bool & con_v = std::get<std::unique_ptr<Boolean>>(con.condition)->value;
    auto & new_v = con_v ? con.if_true : con.if_false;
    ir->instructions.splice(ir->instructions.end(), new_v->instructions);
    ir->condition = std::move(new_v->condition);

    return true;
};

bool machine_lower(IRList * ir,
                   const Meson::Machines::PerMachine<Meson::Machines::Info> & machines) {
    bool progress = false;

    auto it = ir->instructions.begin();
    while (it != ir->instructions.end()) {
        auto & i = *it;
        if (std::holds_alternative<std::unique_ptr<MIR::FunctionCall>>(i)) {
            const auto & f = std::get<std::unique_ptr<MIR::FunctionCall>>(i);
            const auto & holder = f->holder.value_or("");

            Meson::Machines::Machine m;
            if (holder == "build_machine") {
                m = Meson::Machines::Machine::BUILD;
            } else if (holder == "host_machine") {
                m = Meson::Machines::Machine::HOST;
            } else if (holder == "target_machine") {
                m = Meson::Machines::Machine::TARGET;
            } else {
                ++it;
                continue;
            }

            const auto & info = machines.get(m);

            MIR::Object new_value;
            if (f->name == "cpu_family") {
                // TODO: it's probably going to be useful to have a helper for this...
                new_value = MIR::Object{std::make_unique<MIR::String>(info.cpu_family)};
            } else if (f->name == "cpu") {
                // TODO: it's probably going to be useful to have a helper for this...
                new_value = MIR::Object{std::make_unique<MIR::String>(info.cpu)};
            } else if (f->name == "system") {
                // TODO: it's probably going to be useful to have a helper for this...
                new_value = MIR::Object{std::make_unique<MIR::String>(info.system())};
            } else {
                ++it;
                continue;
            }
            // TODO: endian

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
