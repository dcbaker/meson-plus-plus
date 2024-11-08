// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <cassert>

#include "exceptions.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

bool ConstantPropagation::update_data(Instruction & obj) {
    if (!std::holds_alternative<Identifier>(*obj.obj_ptr) &&
        !std::holds_alternative<Phi>(*obj.obj_ptr) &&
        !std::holds_alternative<FunctionCall>(*obj.obj_ptr)) {

        if (obj.var) {
            data.emplace(obj.var, &obj);
        }
    }

    return false;
}

std::optional<Instruction> ConstantPropagation::get(const Identifier & id) const {
    const Variable var{id.value, id.version};
    if (const auto & val = data.find(var); val != data.end()) {
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
              std::holds_alternative<Message>(obj) && std::holds_alternative<FunctionCall>(obj) &&
              std::holds_alternative<std::monostate>(obj))) {
            throw Util::Exceptions::MesonException(
                "Missing MIR type, this is an implementation bug");
        }
#endif
    }
    return std::nullopt;
}

std::optional<Instruction> ConstantPropagation::impl(const Instruction & obj) const {
    auto * id = std::get_if<Identifier>(obj.obj_ptr.get());
    if (id != nullptr && !obj.var) {
        return get(*id);
    }
    return std::nullopt;
}

bool ConstantPropagation::impl(Instruction & obj) const {
    bool progress = false;

    auto * func = std::get_if<FunctionCall>(obj.obj_ptr.get());
    if (func != nullptr) {
        if (auto * holder = std::get_if<Identifier>(func->holder.obj_ptr.get())) {
            if (auto v = get(*holder)) {
                func->holder = v.value();
                progress |= true;
            }
        }
    }

    return progress;
}

bool ConstantPropagation::operator()(std::shared_ptr<CFGNode> block) {
    // We have to break this into two walkers because we need to run this furst,
    // then the replacement
    bool progress =
        instruction_walker(*block, {
                                       [this](Instruction & obj) { return this->update_data(obj); },
                                   });

    progress |= instruction_walker(*block, {[this](Instruction & obj) { return this->impl(obj); }},
                                   {[this](const Instruction & obj) { return this->impl(obj); }});

    return progress;
}

} // namespace MIR::Passes
