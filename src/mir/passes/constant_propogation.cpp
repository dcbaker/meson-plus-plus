// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include <cassert>

#include "exceptions.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

bool ConstantPropagation::update_data(Object & obj) {
    if (!std::holds_alternative<IdentifierPtr>(obj) && !std::holds_alternative<PhiPtr>(obj) &&
        !std::holds_alternative<FunctionCallPtr>(obj)) {

        if (auto v = std::visit(VariableGetter{}, obj)) {
            data.emplace(v, &obj);
        }
    }

    return false;
}

std::optional<Object> ConstantPropagation::get(const IdentifierPtr & id) const {
    const Variable var{id->value, id->version};
    if (const auto & val = data.find(var); val != data.end()) {
        auto & obj = *val->second;
        if (std::holds_alternative<NumberPtr>(obj) || std::holds_alternative<StringPtr>(obj) ||
            std::holds_alternative<BooleanPtr>(obj) || std::holds_alternative<ArrayPtr>(obj) ||
            std::holds_alternative<DictPtr>(obj) || std::holds_alternative<CompilerPtr>(obj) ||
            std::holds_alternative<FilePtr>(obj) || std::holds_alternative<ExecutablePtr>(obj) ||
            std::holds_alternative<StaticLibraryPtr>(obj) ||
            std::holds_alternative<ProgramPtr>(obj) ||
            std::holds_alternative<IncludeDirectoriesPtr>(obj) ||
            std::holds_alternative<CustomTargetPtr>(obj) ||
            std::holds_alternative<DependencyPtr>(obj)) {
            return obj;
        }
#ifndef NDEBUG
        if (!(std::holds_alternative<PhiPtr>(obj) && std::holds_alternative<IdentifierPtr>(obj) &&
              std::holds_alternative<MessagePtr>(obj) &&
              std::holds_alternative<FunctionCallPtr>(obj))) {
            throw Util::Exceptions::MesonException(
                "Missing MIR type, this is an implementation bug");
        }
#endif
    }
    return std::nullopt;
}

std::optional<Object> ConstantPropagation::impl(const Object & obj) const {
    if (std::holds_alternative<IdentifierPtr>(obj)) {
        const auto & id = std::get<IdentifierPtr>(obj);
        if (!std::visit(VariableGetter{}, obj)) {
            return get(id);
        }
    }
    return std::nullopt;
}

bool ConstantPropagation::impl(Object & obj) const {
    bool progress = false;

    if (std::holds_alternative<FunctionCallPtr>(obj)) {
        const auto & func = std::get<FunctionCallPtr>(obj);
        if (func->holder && std::holds_alternative<IdentifierPtr>(func->holder.value())) {
            if (auto v = get(std::get<IdentifierPtr>(func->holder.value()))) {
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
                                       [this](Object & obj) { return this->update_data(obj); },
                                   });

    progress |= instruction_walker(*block, {[this](Object & obj) { return this->impl(obj); }},
                                   {[this](const Object & obj) { return this->impl(obj); }});

    return progress;
}

} // namespace MIR::Passes
