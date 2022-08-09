// SPDX-license-identifier: Apache-2.0
// Copyright © 2022 Dylan Baker

#include "arguments.hpp"

namespace MIR::Arguments {

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

} // namespace MIR::Arguments
