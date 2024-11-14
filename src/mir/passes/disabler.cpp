// SPDX-License-Identifier: Apache-2.0
// Copyright © 2024 Intel Corporation

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

bool is_disabler(const Instruction & it) {
    if (auto * a = std::get_if<MIR::Array>(it.obj_ptr.get())) {
        for (auto & a : a->value) {
            if (is_disabler(a)) {
                return true;
            }
        }
    } else if (auto * d = std::get_if<MIR::Dict>(it.obj_ptr.get())) {
        for (auto & [_, v] : d->value) {
            if (is_disabler(v)) {
                return true;
            }
        }
    } else if (auto * f = std::get_if<MIR::FunctionCall>(it.obj_ptr.get())) {
        for (auto & p : f->pos_args) {
            if (is_disabler(p)) {
                return true;
            }
        }
        for (auto & [_, v] : f->kw_args) {
            if (is_disabler(v)) {
                return true;
            }
        }
        if (is_disabler(f->holder)) {
            return true;
        }
    } else if (auto * j = std::get_if<MIR::Jump>(it.obj_ptr.get())) {
        if (j->predicate) {
            if (is_disabler(*j->predicate)) {
                return true;
            }
        }
    } else if (auto * b = std::get_if<MIR::Branch>(it.obj_ptr.get())) {
        for (auto & [i, _] : b->branches) {
            if (is_disabler(i)) {
                return true;
            }
        }
    }
    return std::holds_alternative<Disabler>(it.object());
}

} // namespace

std::optional<Instruction> disable(const Instruction & obj) {
    if (is_disabler(obj)) {
        return Disabler{};
    }
    return std::nullopt;
}

} // namespace MIR::Passes
