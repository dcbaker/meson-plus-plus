// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "operation.hpp"

#include <sstream>
#include <stdexcept>

namespace MIR::IR {

namespace {

std::string to_string(OperationType t) {
    switch (t) {
        case OperationType::get_attribute:
            return "getattr";
        default:
            throw std::runtime_error("Unknown operation type");
    }
}

} // namespace

Operation::Operation(Instruction l, OperationType t, Instruction r)
    : left{std::move(l)}, type{t}, right{std::move(r)} {};

std::string Operation::serialize() const {
    std::stringstream ss{};
    ss << "Operation { "
       << "left = { " << left.serialize() << " }"
       << "type = { " << to_string(type) << " }"
       << "right = { " << right.serialize() << " }"
       << " }";
    return ss.str();
}

} // namespace MIR::IR
