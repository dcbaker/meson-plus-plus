// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "state.hpp"

#include <sstream>
#include <stdexcept>

namespace MIR::IR {

namespace {

std::string to_string(StateType t) {
    switch (t) {
        case StateType::project_comp_arguments:
            return "project_comp_arguments";
        case StateType::project_link_arguments:
            return "project_link_arguments";
        case StateType::global_comp_arguments:
            return "global_comp_arguments";
        case StateType::global_link_arguments:
            return "global_link_arguments";
        default:
            throw std::runtime_error("Invalid enum type!");
    }
}

} // namespace

State::State(StateType s, PositionalArguments p, KeywordArguments k)
    : type{s}, p_args{std::move(p)}, k_args{std::move(k)} {};

std::string State::serialize() const {
    std::stringstream ss{};
    ss << "State { " << "type = { " << to_string(type) << " } "
       << "positional_arguments = { " << to_string(p_args) << "} "
       << "keyword_arguments = { " << to_string(k_args) << "} "
       << "}";

    return ss.str();
}

} // namespace MIR::IR
