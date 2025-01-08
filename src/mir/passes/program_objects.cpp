// SPDX-License-Identifier: Apache-2.0
// Copyright © 2022-2025 Intel Corporation

#include "exceptions.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

std::optional<Object> lower_found_method(const FunctionCallPtr & f) {
    if (!f->pos_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Program.found() does not take any positional arguments");
    }
    if (!f->kw_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Program.found() does not take any keyword arguments");
    }

    assert(f->holder.has_value());
    return std::make_shared<Boolean>(std::get<ProgramPtr>(f->holder.value())->found());
}

} // namespace

std::optional<Object> lower_program_objects(const Object & obj, const State::Persistant & pstate) {
    if (!std::holds_alternative<FunctionCallPtr>(obj)) {
        return std::nullopt;
    }
    const auto & f = std::get<FunctionCallPtr>(obj);

    if (!(f->holder && std::holds_alternative<ProgramPtr>(f->holder.value()))) {
        return std::nullopt;
    }

    if (!all_args_reduced(f->pos_args, f->kw_args)) {
        return std::nullopt;
    }

    std::optional<Object> i = std::nullopt;
    if (f->name == "found") {
        i = lower_found_method(f);
    }
    if (i) {
        const std::optional<Variable> var = std::visit(VariableGetter{}, obj);
        if (var) {
            std::visit(VariableSetter{var.value()}, i.value());
        }
        return i;
    }
    // XXX: Shouldn't really be able to get here...
    return std::nullopt;
}

} // namespace MIR::Passes
