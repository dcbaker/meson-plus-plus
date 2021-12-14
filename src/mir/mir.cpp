// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "mir.hpp"
#include "exceptions.hpp"

namespace MIR {

namespace {

static uint32_t bb_index = 0;

}

bool Phi::operator==(const Phi & other) const {
    return var.name == other.var.name && left == other.left && right == other.right;
}

bool Phi::operator<(const Phi & other) const {
    return var.name < other.var.name && left < other.left && right < other.right;
}

BasicBlock::BasicBlock()
    : instructions{}, next{std::monostate{}}, parents{}, index{++bb_index}, variables{} {};

BasicBlock::BasicBlock(std::unique_ptr<Condition> && con)
    : instructions{}, next{std::move(con)}, parents{}, index{++bb_index}, variables{} {};

void BasicBlock::update_variables(bool clear) {
    if (clear) {
        variables.clear();
    }

    // It's convenient to not have to lookup variables up the tree, and instead
    // have all variables for a particular path. It's fine that we're
    // overwritting values here, the'll all be overwritten in the next block anyway.
    for (const auto & p : parents) {
        for (const auto & [name, obj] : p->variables) {
            variables[name] = obj;
        }
    }

    for (const auto & obj : instructions) {
        if (const auto & var = std::visit([](const auto & obj) { return obj->var; }, obj); var) {
            variables[var.name] = &obj;
        }
    }
}

Condition::Condition(Object && o)
    : condition{std::move(o)}, if_true{std::make_shared<BasicBlock>()}, if_false{nullptr} {};

Condition::Condition(Object && o, std::shared_ptr<BasicBlock> s)
    : condition{std::move(o)}, if_true{s}, if_false{nullptr} {};

const Object Compiler::get_id(const std::vector<Object> & args,
                              const std::unordered_map<std::string, Object> & kwargs) const {
    if (!args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "compiler.get_id(): takes no positional arguments");
    }
    if (!kwargs.empty()) {
        throw Util::Exceptions::InvalidArguments("compiler.get_id(): takes no keyword arguments");
    }

    return std::make_unique<String>(toolchain->compiler->id());
};

Variable::operator bool() const { return !name.empty(); };

} // namespace MIR
