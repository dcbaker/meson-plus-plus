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
            "Dependency.found() does not take any positional arguments");
    }
    if (!f->kw_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Dependency.found() does not take any keyword arguments");
    }

    return std::make_shared<Boolean>(
        std::get<DependencyPtr>(f->holder.value())->found);
}

std::optional<Object> lower_version_method(const FunctionCallPtr & f) {
    if (!f->pos_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Dependency.version() does not take any positional arguments");
    }
    if (!f->kw_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Dependency.version() does not take any keyword arguments");
    }

    return std::make_shared<String>(
        std::get<DependencyPtr>(f->holder.value())->version);
}

std::optional<Object> lower_name_method(const FunctionCallPtr & f) {
    if (!f->pos_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Dependency.name() does not take any positional arguments");
    }
    if (!f->kw_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Dependency.name() does not take any keyword arguments");
    }

    return std::make_shared<String>(std::get<DependencyPtr>(f->holder.value())->name);
}

} // namespace

std::optional<Object> lower_dependency_objects(const Object & obj,
                                               const State::Persistant & pstate) {
    if (!std::holds_alternative<FunctionCallPtr>(obj)) {
        return std::nullopt;
    }
    const auto & f = std::get<FunctionCallPtr>(obj);

    if (!f->holder || !std::holds_alternative<DependencyPtr>(f->holder.value())) {
        return std::nullopt;
    }

    if (!all_args_reduced(f->pos_args, f->kw_args)) {
        return std::nullopt;
    }

    if (f->name == "found") {
        return lower_found_method(f);
    }
    if (f->name == "version") {
        return lower_version_method(f);
    }
    if (f->name == "name") {
        return lower_name_method(f);
    }

    // XXX: Shouldn't really be able to get here...
    return std::nullopt;
}

} // namespace MIR::Passes
