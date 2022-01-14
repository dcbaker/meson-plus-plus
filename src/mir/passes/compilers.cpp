// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include <stdexcept>

#include "exceptions.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

inline bool valid_holder(const std::optional<Object> & holder) {
    if (!holder) {
        return false;
    } else if (!std::holds_alternative<std::unique_ptr<Identifier>>(holder.value())) {
        return false;
    } else {
        return std::get<std::unique_ptr<Identifier>>(holder.value())->value == "meson";
    }
}

using ToolchainMap =
    std::unordered_map<MIR::Toolchain::Language,
                       MIR::Machines::PerMachine<std::shared_ptr<MIR::Toolchain::Toolchain>>>;

std::optional<Object> replace_compiler(const Object & obj, const ToolchainMap & tc) {
    if (!std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        return std::nullopt;
    }
    const auto & f = std::get<std::shared_ptr<FunctionCall>>(obj);

    // XXX: this seems like a bug
    if (f.get() == nullptr) {
        return std::nullopt;
    }

    if (!(valid_holder(f->holder) && f->name == "get_compiler")) {
        return std::nullopt;
    }

    // XXX: if there is no argument here this is going to blow up spectacularly
    const auto & l = f->pos_args[0];
    // If we haven't reduced this to a string then we need to wait and try again later
    if (!std::holds_alternative<std::shared_ptr<String>>(l)) {
        return std::nullopt;
    }

    const auto & lang = MIR::Toolchain::from_string(std::get<std::shared_ptr<String>>(l)->value);

    MIR::Machines::Machine m;
    try {
        const auto & n = f->kw_args.at("native");
        // If we haven't lowered this away yet, then we can't reduce this.
        if (!std::holds_alternative<std::shared_ptr<Boolean>>(n)) {
            return std::nullopt;
        }
        const auto & native = std::get<std::shared_ptr<Boolean>>(n)->value;

        m = native ? MIR::Machines::Machine::BUILD : MIR::Machines::Machine::HOST;
    } catch (std::out_of_range &) {
        m = MIR::Machines::Machine::HOST;
    }

    try {
        return std::make_unique<Compiler>(tc.at(lang).get(m));
    } catch (std::out_of_range &) {
        // TODO: add a better error message
        throw Util::Exceptions::MesonException{"No compiler for language"};
    }
}

} // namespace

bool insert_compilers(BasicBlock * block, const ToolchainMap & toolchains) {
    auto cb = [&](const Object & obj) { return replace_compiler(obj, toolchains); };
    return function_walker(block, cb);
};

} // namespace MIR::Passes
