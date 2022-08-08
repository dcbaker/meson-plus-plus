// SPDX-license-identifier: Apache-2.0
// Copyright © 2022 Dylan Baker

#include "arguments.hpp"

namespace MIR::Arguments {

Argument::Argument(std::string v, const Type & t)
    : value{std::move(v)}, type{t}, inc_type{IncludeType::BASE} {};
Argument::Argument(std::string v, const Type & t, const IncludeType & i)
    : value{std::move(v)}, type{t}, inc_type{i} {};

bool Argument::operator==(const Argument & other) const {
    return type == other.type && inc_type == other.inc_type && value == other.value;
}

} // namespace MIR::Arguments
