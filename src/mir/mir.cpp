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

BasicBlock::BasicBlock() : instructions{}, next{std::monostate{}}, parents{}, index{++bb_index} {};

BasicBlock::BasicBlock(std::unique_ptr<Condition> && con)
    : instructions{}, next{std::move(con)}, parents{}, index{++bb_index} {};

bool BasicBlock::operator<(const BasicBlock & other) const { return index < other.index; }

bool BBComparitor::operator()(const BasicBlock * lhs, const BasicBlock * rhs) const {
    return *lhs < *rhs;
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

    return std::make_shared<String>(toolchain->compiler->id());
};

Variable::operator bool() const { return !name.empty(); };

bool Variable::operator<(const Variable & other) const {
    return name < other.name && version < other.version;
}

bool Variable::operator==(const Variable & other) const {
    return name == other.name && version == other.version;
}

} // namespace MIR
