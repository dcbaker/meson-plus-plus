// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include <cassert>

#include "exceptions.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

bool identifier_to_object_mapper(Object & obj, PropTable & table) {
    if (!std::holds_alternative<std::unique_ptr<Identifier>>(obj) &&
        !std::holds_alternative<std::unique_ptr<Phi>>(obj) &&
        !std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        const Variable & var = std::visit([](const auto & o) { return o->var; }, obj);
        if (var) {
            table[var] = &obj;
        }
    }

    return false;
}

std::optional<Object> get_value(const Identifier & id, const PropTable & table) {
    const Variable var{id.value, id.version};
    if (const auto & val = table.find(var); val != table.end()) {
        auto & v = *val->second;
        if (std::holds_alternative<std::shared_ptr<Number>>(v)) {
            return std::get<std::shared_ptr<Number>>(v);
        } else if (std::holds_alternative<std::shared_ptr<String>>(v)) {
            return std::get<std::shared_ptr<String>>(v);
        } else if (std::holds_alternative<std::shared_ptr<Boolean>>(v)) {
            return std::get<std::shared_ptr<Boolean>>(v);
        } else if (std::holds_alternative<std::shared_ptr<Array>>(v)) {
            return std::get<std::shared_ptr<Array>>(v);
        } else if (std::holds_alternative<std::shared_ptr<Dict>>(v)) {
            return std::get<std::shared_ptr<Dict>>(v);
        } else if (std::holds_alternative<std::shared_ptr<Compiler>>(v)) {
            return std::get<std::shared_ptr<Compiler>>(v);
        } else if (std::holds_alternative<std::shared_ptr<File>>(v)) {
            return std::get<std::shared_ptr<File>>(v);
        } else if (std::holds_alternative<std::shared_ptr<Executable>>(v)) {
            return std::get<std::shared_ptr<Executable>>(v);
        } else if (std::holds_alternative<std::shared_ptr<StaticLibrary>>(v)) {
            return std::get<std::shared_ptr<StaticLibrary>>(v);
        } else if (std::holds_alternative<std::shared_ptr<Program>>(v)) {
            return std::get<std::shared_ptr<Program>>(v);
        } else if (std::holds_alternative<std::shared_ptr<IncludeDirectories>>(v)) {
            return std::get<std::shared_ptr<IncludeDirectories>>(v);
        } else if (std::holds_alternative<std::shared_ptr<CustomTarget>>(v)) {
            return std::get<std::shared_ptr<CustomTarget>>(v);
#ifndef NDEBUG
        } else if (!(std::holds_alternative<std::unique_ptr<Phi>>(v) &&
                     std::holds_alternative<std::unique_ptr<Identifier>>(v) &&
                     std::holds_alternative<std::unique_ptr<Empty>>(v) &&
                     std::holds_alternative<std::unique_ptr<Message>>(v) &&
                     std::holds_alternative<std::shared_ptr<FunctionCall>>(v))) {
            throw Util::Exceptions::MesonException(
                "Missing MIR type, this is an implementation bug");
#endif
        }
    }
    return std::nullopt;
}

std::optional<Object> constant_propogation_impl(const Object & obj, const PropTable & table) {
    if (std::holds_alternative<std::unique_ptr<Identifier>>(obj)) {
        const auto & id = std::get<std::unique_ptr<Identifier>>(obj);
        if (!id->var) {
            return get_value(*id, table);
        }
    }

    return std::nullopt;
}

bool constant_propogation_holder_impl(Object & obj, const PropTable & table) {
    bool progress = false;

    if (std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        const auto & func = std::get<std::shared_ptr<FunctionCall>>(obj);
        if (func->holder) {
            const auto & id = std::get<std::unique_ptr<Identifier>>(func->holder.value());
            auto v = get_value(*id, table);
            if (v) {
                func->holder = std::move(v);
            }
        }
    }

    return progress;
}

} // namespace

bool constant_propogation(BasicBlock * block, PropTable & table) {
    const auto & prop = [&](const Object & obj) { return constant_propogation_impl(obj, table); };
    const auto & prop_h = [&](Object & obj) {
        return constant_propogation_holder_impl(obj, table);
    };

    // We have to break this into two walkers because we need to run this furst,
    // then the replacement
    bool progress = instruction_walker(
        block, {
                   [&](Object & obj) { return identifier_to_object_mapper(obj, table); },
               });

    progress |= instruction_walker(
        block,
        {
            [&](const Object & obj) { return array_walker(obj, prop); },
            [&](Object & obj) { return array_walker(obj, prop_h); },
            [&](const Object & obj) { return function_argument_walker(obj, prop); },
            [&](Object & obj) { return function_argument_walker(obj, prop_h); },
            prop_h,
        },
        {
            prop,
        });

    return progress;
}

} // namespace MIR::Passes
