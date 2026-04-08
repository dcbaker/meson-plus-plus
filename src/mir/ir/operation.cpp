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
        case Operation1SrcType::lnot:
            return "logical not";
        case Operation1SrcType::negate:
            return "negation";
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
        case Operation2SrcType::lt:
            return "less than";
        case Operation2SrcType::le:
            return "less than or equal";
        case Operation2SrcType::eq:
            return "equal";
        case Operation2SrcType::ne:
            return "not equal";
        case Operation2SrcType::ge:
            return "greater than or equal";
        case Operation2SrcType::gt:
            return "greater than";
        case Operation2SrcType::not_in:
            return "not in";
        case Operation2SrcType::in:
            return "in";
        case Operation2SrcType::and_:
            return "and";
        case Operation2SrcType::or_:
            return "or";
        case Operation2SrcType::mod:
            return "mod";
        case Operation2SrcType::mul:
            return "multiply";
        case Operation2SrcType::div:
            return "division";
        case Operation2SrcType::add:
            return "addition";
        case Operation2SrcType::sub:
            return "subtraction";
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
