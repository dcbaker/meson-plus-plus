// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <stdexcept>

#include "exceptions.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

inline bool valid_holder(const std::optional<Instruction> & holder) {
    if (!holder) {
        return false;
    }
    auto && held = holder.value();

    if (!std::holds_alternative<Identifier>(*held.obj_ptr)) {
        return false;
    }
    return std::get<Identifier>(*held.obj_ptr).value == "meson";
}

using ToolchainMap =
    std::unordered_map<MIR::Toolchain::Language,
                       MIR::Machines::PerMachine<std::shared_ptr<MIR::Toolchain::Toolchain>>>;

std::optional<Instruction> replace_compiler(const Instruction & obj, const ToolchainMap & tc) {
    if (!std::get_if<FunctionCall>(obj.obj_ptr.get())) {
        return std::nullopt;
    }
    const auto & f = std::get<FunctionCall>(*obj.obj_ptr);

    if (!(valid_holder(f.holder) && f.name == "get_compiler")) {
        return std::nullopt;
    }

    // XXX: if there is no argument here this is going to blow up spectacularly
    const auto & l = f.pos_args[0];
    // If we haven't reduced this to a string then we need to wait and try again later
    if (!std::get_if<String>(l.obj_ptr.get())) {
        return std::nullopt;
    }

    const auto & lang = MIR::Toolchain::from_string(std::get<String>(*l.obj_ptr).value);

    MIR::Machines::Machine m;
    try {
        const auto & n = f.kw_args.at("native");
        // If we haven't lowered this away yet, then we can't reduce this.
        if (!std::get_if<Boolean>(n.obj_ptr.get())) {
            return std::nullopt;
        }
        const auto & native = std::get<Boolean>(*n.obj_ptr).value;

        m = native ? MIR::Machines::Machine::BUILD : MIR::Machines::Machine::HOST;
    } catch (std::out_of_range &) {
        m = MIR::Machines::Machine::HOST;
    }

    try {
        return std::make_shared<Object>(Compiler{tc.at(lang).get(m)});
    } catch (std::out_of_range &) {
        // TODO: add a better error message
        throw Util::Exceptions::MesonException{"No compiler for language"};
    }
}

} // namespace

bool insert_compilers(BasicBlock & block, const ToolchainMap & toolchains) {
    auto cb = [&](const Instruction & obj) { return replace_compiler(obj, toolchains); };
    return function_walker(block, cb);
};

} // namespace MIR::Passes
