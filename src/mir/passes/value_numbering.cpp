// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

bool number(Object & obj, std::unordered_map<std::string, uint32_t> & data) {
    Variable * var = std::visit([](auto & obj) { return &obj->var; }, obj);
    if (!var) {
        return false;
    }

    // We never revalue a number, otherwise we might end up in a situation where
    // we change the number in the assignment, but then the users point to the
    // wrong thing
    if (var->version > 0) {
        return false;
    }

    if (data.count(var->name) == 0) {
        data[var->name] = 0;
    }

    var->version = ++data[var->name];

    return true;
}

} // namespace

bool value_numbering(BasicBlock * block, std::unordered_map<std::string, uint32_t> & data) {
    return instruction_walker(block, {[&](Object & obj) { return number(obj, data); }});
}

} // namespace MIR::Passes
