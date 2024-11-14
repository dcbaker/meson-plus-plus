// SPDX-License-Identifier: Apache-2.0
// Copyright © 2022-2024 Intel Corporation

#include "exceptions.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

std::optional<Instruction> lower_found_method(const FunctionCall & f) {
    if (!f.pos_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Program.found() does not take any positional arguments");
    }
    if (!f.kw_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Program.found() does not take any keyword arguments");
    }

    return Boolean{std::get<Program>(*f.holder.obj_ptr).found()};
}

} // namespace

std::optional<Instruction> lower_program_objects(const Instruction & obj,
                                                 const State::Persistant & pstate) {
    if (!std::holds_alternative<FunctionCall>(*obj.obj_ptr)) {
        return std::nullopt;
    }
    const auto & f = std::get<FunctionCall>(*obj.obj_ptr);

    if (!std::holds_alternative<Program>(*f.holder.obj_ptr)) {
        return std::nullopt;
    }

    if (!all_args_reduced(f.pos_args, f.kw_args)) {
        return std::nullopt;
    }

    std::optional<Instruction> i = std::nullopt;
    if (f.name == "found") {
        i = lower_found_method(f);
    }
    if (i) {
        i.value().var = obj.var;
        return i;
    }
    // XXX: Shouldn't really be able to get here...
    return std::nullopt;
}

} // namespace MIR::Passes
