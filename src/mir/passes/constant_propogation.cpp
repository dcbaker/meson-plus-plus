// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include <cassert>

#include "exceptions.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

bool identifier_to_object_mapper(Instruction & obj, PropTable & table) {
    if (!std::holds_alternative<Identifier>(*obj.obj_ptr) &&
        !std::holds_alternative<Phi>(*obj.obj_ptr) &&
        !std::holds_alternative<FunctionCall>(*obj.obj_ptr)) {

        if (obj.var) {
            table[obj.var] = &obj;
        }
    }

    return false;
}

std::optional<Instruction> get_value(const Identifier & id, const PropTable & table) {
    const Variable var{id.value, id.version};
    if (const auto & val = table.find(var); val != table.end()) {
        auto & v = *val->second;
        const Object & obj = *v.obj_ptr;
        if (std::holds_alternative<Number>(obj) || std::holds_alternative<String>(obj) ||
            std::holds_alternative<Boolean>(obj) || std::holds_alternative<Array>(obj) ||
            std::holds_alternative<Dict>(obj) || std::holds_alternative<Compiler>(obj) ||
            std::holds_alternative<File>(obj) || std::holds_alternative<Executable>(obj) ||
            std::holds_alternative<StaticLibrary>(obj) || std::holds_alternative<Program>(obj) ||
            std::holds_alternative<IncludeDirectories>(obj) ||
            std::holds_alternative<CustomTarget>(obj) || std::holds_alternative<Dependency>(obj)) {
            return v;
        }
#ifndef NDEBUG
        if (!(std::holds_alternative<Phi>(obj) && std::holds_alternative<Identifier>(obj) &&
              std::holds_alternative<Empty>(obj) && std::holds_alternative<Message>(obj) &&
              std::holds_alternative<FunctionCall>(obj) &&
              std::holds_alternative<std::monostate>(obj))) {
            throw Util::Exceptions::MesonException(
                "Missing MIR type, this is an implementation bug");
        }
#endif
    }
    return std::nullopt;
}

std::optional<Instruction> constant_propogation_impl(const Instruction & obj,
                                                     const PropTable & table) {
    auto * id = std::get_if<Identifier>(obj.obj_ptr.get());
    if (id != nullptr && !obj.var) {
        return get_value(*id, table);
    }
    return std::nullopt;
}

bool constant_propogation_holder_impl(Instruction & obj, const PropTable & table) {
    bool progress = false;

    auto * func = std::get_if<FunctionCall>(obj.obj_ptr.get());
    if (func != nullptr) {
        auto * holder = std::get_if<Identifier>(func->holder.obj_ptr.get());
        if (holder != nullptr) {
            if (auto v = get_value(*holder, table)) {
                func->holder = v.value();
            }
        }
    }

    return progress;
}

} // namespace

bool constant_propogation(BasicBlock & block, PropTable & table) {
    const auto & prop = [&](const Instruction & obj) {
        return constant_propogation_impl(obj, table);
    };
    const auto & prop_h = [&](Instruction & obj) {
        return constant_propogation_holder_impl(obj, table);
    };

    // We have to break this into two walkers because we need to run this furst,
    // then the replacement
    bool progress = instruction_walker(
        block, {
                   [&](Instruction & obj) { return identifier_to_object_mapper(obj, table); },
               });

    progress |= instruction_walker(
        block,
        {
            [&](const Instruction & obj) { return array_walker(obj, prop); },
            [&](Instruction & obj) { return array_walker(obj, prop_h); },
            [&](const Instruction & obj) { return function_argument_walker(obj, prop); },
            [&](Instruction & obj) { return function_argument_walker(obj, prop_h); },
            prop_h,
        },
        {
            prop,
        });

    return progress;
}

} // namespace MIR::Passes
