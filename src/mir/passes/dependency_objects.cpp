// SPDX-license-identifier: Apache-2.0
// Copyright © 2022 Dylan Baker

#include "exceptions.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

std::optional<Object> lower_found_method(const FunctionCall & f) {
    if (!f.pos_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Dependency.found() does not take any positional arguments");
    }
    if (!f.kw_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Dependency.found() does not take any keyword arguments");
    }

    return std::make_shared<Boolean>(
        std::get<std::shared_ptr<Dependency>>(f.holder.value())->found);
}

std::optional<Object> lower_version_method(const FunctionCall & f) {
    if (!f.pos_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Dependency.version() does not take any positional arguments");
    }
    if (!f.kw_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Dependency.version() does not take any keyword arguments");
    }

    return std::make_shared<String>(
        std::get<std::shared_ptr<Dependency>>(f.holder.value())->version);
}

std::optional<Object> lower_name_method(const FunctionCall & f) {
    if (!f.pos_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Dependency.name() does not take any positional arguments");
    }
    if (!f.kw_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Dependency.name() does not take any keyword arguments");
    }

    return std::make_shared<String>(std::get<std::shared_ptr<Dependency>>(f.holder.value())->name);
}

std::optional<Object> lower_dependency_methods_impl(const Object & obj,
                                                    const State::Persistant & pstate) {
    if (!std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        return std::nullopt;
    }
    const auto & f = *std::get<std::shared_ptr<FunctionCall>>(obj);

    if (!(f.holder.has_value() &&
          std::holds_alternative<std::shared_ptr<Dependency>>(f.holder.value()))) {
        return std::nullopt;
    }

    if (!all_args_reduced(f.pos_args, f.kw_args)) {
        return std::nullopt;
    }

    if (f.name == "found") {
        return lower_found_method(f);
    } else if (f.name == "version") {
        return lower_version_method(f);
    } else if (f.name == "name") {
        return lower_name_method(f);
    }

    // XXX: Shouldn't really be able to get here...
    return std::nullopt;
}

} // namespace

bool lower_dependency_objects(BasicBlock & block, State::Persistant & pstate) {
    return function_walker(
        &block, [&](const Object & obj) { return lower_dependency_methods_impl(obj, pstate); });
}

} // namespace MIR::Passes
