// SPDX-License-Identifier: Apache-2.0
// Copyright © 2022-2024 Intel Corporation

#include "arguments.hpp"

#include <cassert>
#include <sstream>

namespace MIR::Arguments {

namespace {

std::string to_string(const Type type) {
    switch (type) {
        case Type::DEFINE:
            return "Pre-processor define";
        case Type::INCLUDE:
            return "Include Directories";
        case Type::LINK:
            return "Dynamic Linker Arguments";
        case Type::LINK_SEARCH:
            return "Linker Search paths";
        case Type::RAW:
            return "Raw compiler/linker argument";
        default:
            assert(0);
    }
}

std::string to_string(const IncludeType type) {
    switch (type) {
        case IncludeType::BASE:
            return "normal";
        case IncludeType::SYSTEM:
            return "system";
        default:
            assert(0);
    }
}

} // namespace

Argument::Argument(std::string v, const Type & t)
    : _value{std::move(v)}, _type{t}, inc_type{IncludeType::BASE} {};
Argument::Argument(std::string v, const Type & t, const IncludeType & i)
    : _value{std::move(v)}, _type{t}, inc_type{i} {};

bool Argument::operator==(const Argument & other) const {
    return type() == other.type() && include_type() == other.include_type() &&
           value() == other.value();
}

std::string Argument::value() const { return _value; }

Type Argument::type() const { return _type; }

IncludeType Argument::include_type() const { return inc_type; }

std::string Argument::print() const {
    std::stringstream ss{};
    ss << "Argument = { type = { " << to_string(_type) << " }, value = { " << _value << " } ";
    if (_type == Type::INCLUDE) {
        ss << ", include_type = { " << to_string(inc_type) << " }";
    }
    ss << " }";
    return ss.str();
}

} // namespace MIR::Arguments
