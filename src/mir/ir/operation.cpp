// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "operation.hpp"
#include "instructions.hpp"

#include <sstream>
#include <stdexcept>

namespace MIR::IR {

namespace {

std::string to_string(OperationType t) {
    switch (t) {
        case OperationType::get_attribute:
            return "getattr";
        case OperationType::subscript:
            return "subscript";
        default:
            throw std::runtime_error("Unknown operation type");
    }
}

} // namespace

Operation::Operation(InstructionType l, OperationType t, InstructionType r)
    : left{std::move(l)}, type{t}, right{std::move(r)} {};

std::string Operation::serialize() const {
    std::stringstream ss{};
    auto && visitor = [](auto && i) -> std::string { return i->serialize(); };
    ss << "Operation { "
       << "left = { " << std::visit(visitor, left) << " } "
       << "type = { " << to_string(type) << " } "
       << "right = { " << std::visit(visitor, right) << " } "
       << "}";
    return ss.str();
}

} // namespace MIR::IR
