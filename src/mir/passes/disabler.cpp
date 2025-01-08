// SPDX-License-Identifier: Apache-2.0
// Copyright © 2024-2025 Intel Corporation

#include "passes.hpp"
#include "private.hpp"

#include <algorithm>

namespace MIR::Passes {

namespace {

bool is_disabler(const Object & it) {
    if (std::holds_alternative<MIR::ArrayPtr>(it)) {
        auto obj = std::get<MIR::ArrayPtr>(it);
        return std::any_of(obj->value.begin(), obj->value.end(),
                           [](const Object & o) { return is_disabler(o); });
    } else if (std::holds_alternative<MIR::DictPtr>(it)) {
        auto obj = std::get<MIR::DictPtr>(it);
        return std::any_of(
            obj->value.begin(), obj->value.end(),
            [](const std::pair<std::string, Object> & o) { return is_disabler(o.second); });
    } else if (std::holds_alternative<MIR::FunctionCallPtr>(it)) {
        auto obj = std::get<MIR::FunctionCallPtr>(it);
        if (obj->holder && is_disabler(obj->holder.value())) {
            return true;
        }
        if (std::any_of(obj->pos_args.begin(), obj->pos_args.end(),
                        [](const Object & o) { return is_disabler(o); })) {
            return true;
        }
        return std::any_of(
            obj->kw_args.begin(), obj->kw_args.end(),
            [](const std::pair<std::string, Object> & o) { return is_disabler(o.second); });
    } else if (std::holds_alternative<MIR::JumpPtr>(it)) {
        auto obj = std::get<MIR::JumpPtr>(it);
        if (obj->predicate && is_disabler(obj->predicate.value())) {
            return true;
        }
    } else if (std::holds_alternative<MIR::BranchPtr>(it)) {
        auto obj = std::get<MIR::BranchPtr>(it);
        return std::any_of(obj->branches.begin(), obj->branches.end(),
                           [](const std::tuple<Object, std::shared_ptr<CFGNode>> & o) {
                               return is_disabler(std::get<0>(o));
                           });
    }
    return std::holds_alternative<DisablerPtr>(it);
}

} // namespace

std::optional<Object> disable(const Object & obj) {
    if (is_disabler(obj)) {
        return std::make_shared<Disabler>();
    }
    return std::nullopt;
}

} // namespace MIR::Passes
