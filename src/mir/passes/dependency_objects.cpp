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
            "Dependency.found() does not take any positional arguments");
    }
    if (!f.kw_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Dependency.found() does not take any keyword arguments");
    }

    return Boolean{std::get<Dependency>(*f.holder.obj_ptr).found};
}

std::optional<Instruction> lower_version_method(const FunctionCall & f) {
    if (!f.pos_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Dependency.version() does not take any positional arguments");
    }
    if (!f.kw_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Dependency.version() does not take any keyword arguments");
    }

    return String{std::get<Dependency>(*f.holder.obj_ptr).version};
}

std::optional<Instruction> lower_name_method(const FunctionCall & f) {
    if (!f.pos_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Dependency.name() does not take any positional arguments");
    }
    if (!f.kw_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Dependency.name() does not take any keyword arguments");
    }

    return String{std::get<Dependency>(*f.holder.obj_ptr).name};
}

std::optional<Instruction> lower_dependency_methods_impl(const Instruction & obj,
                                                         const State::Persistant & pstate) {
    const auto * f = std::get_if<FunctionCall>(obj.obj_ptr.get());
    if (f == nullptr) {
        return std::nullopt;
    }

    if (std::get_if<Dependency>(f->holder.obj_ptr.get()) == nullptr) {
        return std::nullopt;
    }

    if (!all_args_reduced(f->pos_args, f->kw_args)) {
        return std::nullopt;
    }

    if (f->name == "found") {
        return lower_found_method(*f);
    }
    if (f->name == "version") {
        return lower_version_method(*f);
    }
    if (f->name == "name") {
        return lower_name_method(*f);
    }

    // XXX: Shouldn't really be able to get here...
    return std::nullopt;
}

} // namespace

bool lower_dependency_objects(std::shared_ptr<CFGNode> block, State::Persistant & pstate) {
    return instruction_walker(*block, {[&](const Instruction & obj) {
        return lower_dependency_methods_impl(obj, pstate);
    }});
}

} // namespace MIR::Passes
