// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include "argument_extractors.hpp"
#include "exceptions.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

inline bool valid_holder(const std::optional<Object> & holder) {
    if (!holder) {
        return false;
    }
    auto && held = holder.value();

    if (!std::holds_alternative<IdentifierPtr>(held)) {
        return false;
    }
    return std::get<IdentifierPtr>(held)->value == "meson";
}

using ToolchainMap =
    std::unordered_map<MIR::Toolchain::Language,
                       MIR::Machines::PerMachine<std::shared_ptr<MIR::Toolchain::Toolchain>>>;

Object lower_get_id_method(const FunctionCall & func) {
    if (!func.pos_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "compiler.get_id(): takes no positional arguments");
    }
    if (!func.kw_args.empty()) {
        throw Util::Exceptions::InvalidArguments("compiler.get_id(): takes no keyword arguments");
    }

    const auto & comp = std::get<CompilerPtr>(func.holder.value());

    return std::make_shared<String>(comp->toolchain->compiler->id());
}

} // namespace

std::optional<Object> insert_compilers(const Object & obj, const ToolchainMap & tc) {
    if (!std::holds_alternative<FunctionCallPtr>(obj)) {
        return std::nullopt;
    }
    const auto & f = *std::get<FunctionCallPtr>(obj);

    if (!(valid_holder(f.holder) && f.name == "get_compiler")) {
        return std::nullopt;
    }

    if (f.pos_args.size() != 1) {
        throw Util::Exceptions::InvalidArguments(
            "meson.get_compiler(): requires exactly 1 positional argument");
    }

    if (!all_args_reduced(f.pos_args, f.kw_args)) {
        return std::nullopt;
    }

    const std::string l =
        extract_positional_argument<StringPtr>(
            f.pos_args.at(0), "meson.get_compiler(): first argument must be a string")
            ->value;
    const auto & lang = MIR::Toolchain::from_string(l);

    MIR::Machines::Machine m;
    const bool native =
        extract_keyword_argument<BooleanPtr>(f.kw_args, "native", "must be a boolean")
            .value_or(std::make_shared<Boolean>(false))
            ->value;
    m = native ? MIR::Machines::Machine::BUILD : MIR::Machines::Machine::HOST;

    try {
        return std::make_shared<Compiler>(tc.at(lang).get(m));
    } catch (std::out_of_range &) {
        // TODO: add a better error message
        throw Util::Exceptions::MesonException{"No compiler for language '" + l + "'"};
    }
}

std::optional<Object> lower_compiler_methods(const Object & obj) {
    if (!std::holds_alternative<FunctionCallPtr>(obj)) {
        return std::nullopt;
    }
    const auto & f = *std::get<FunctionCallPtr>(obj);

    if (!f.holder || !std::holds_alternative<CompilerPtr>(f.holder.value())) {
        return std::nullopt;
    }

    if (!all_args_reduced(f.pos_args, f.kw_args)) {
        return std::nullopt;
    }

    std::optional<Object> i;
    if (f.name == "get_id") {
        i.emplace(lower_get_id_method(f));
    } else {
        i = std::nullopt;
    }

    if (i) {
        std::visit(MIR::VariableSetter{f.var}, i.value());
        return i;
    }

    // XXX: Shouldn't really be able to get here...
    return std::nullopt;
}

} // namespace MIR::Passes
