// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "operation.hpp"
#include "instructions.hpp"

#include <sstream>
#include <stdexcept>

namespace MIR::IR {

namespace {

std::string to_string(Operation1SrcType t) {
    switch (t) {
        default:
            throw std::runtime_error("Unknown operation type");
    }
}

std::string to_string(Operation2SrcType t) {
    switch (t) {
        case Operation2SrcType::get_attribute:
            return "getattr";
        case Operation2SrcType::subscript:
            return "subscript";
        default:
            throw std::runtime_error("Unknown operation type");
    }
}

} // namespace

Operation1Src::Operation1Src(InstructionType l, Operation1SrcType t)
    : left{std::move(l)}, type{t} {};

std::string Operation1Src::serialize() const {
    std::stringstream ss{};
    auto && visitor = [](auto && i) -> std::string { return i->serialize(); };
    ss << "Operation2Src { "
       << "left = { " << std::visit(visitor, left) << " } "
       << "type = { " << to_string(type) << " } "
       << "}";
    return ss.str();
}

Operation2Src::Operation2Src(InstructionType l, Operation2SrcType t, InstructionType r)
    : left{std::move(l)}, type{t}, right{std::move(r)} {};

std::string Operation2Src::serialize() const {
    std::stringstream ss{};
    auto && visitor = [](auto && i) -> std::string { return i->serialize(); };
    ss << "Operation2Src { "
       << "left = { " << std::visit(visitor, left) << " } "
       << "type = { " << to_string(type) << " } "
       << "right = { " << std::visit(visitor, right) << " } "
       << "}";
    return ss.str();
}

} // namespace MIR::IR
