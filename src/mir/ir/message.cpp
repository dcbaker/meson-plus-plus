// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025-2026 Intel Corporation

#include "message.hpp"
#include "helpers.hpp"
#include "instruction.hpp"

#include <sstream>

namespace MIR::IR {

namespace {

std::string to_string(MessageType t) {
    switch (t) {
        case MessageType::debug:
            return "debug";
        case MessageType::message:
            return "message";
        case MessageType::warning:
            return "warning";
        case MessageType::error:
            return "error";
        default:
            throw std::runtime_error("Invalid Enum type!");
    }
}

} // namespace

using Private::indenter;

Message::Message(MessageType t, std::string s)
    : type{t}, message{std::move(s)}, is_error{t == MessageType::error} {};
Message::Message(MessageType t, std::string s, bool e)
    : type{t}, message{std::move(s)}, is_error{e} {};

std::string Message::serialize(unsigned indent) const {
    std::stringstream ss{};
    ss << indenter(indent) << "Message {\n"
       << indenter(indent + 1) << "type = { " << to_string(type) << " }\n"
       << indenter(indent + 1) << "message = { \"" << message << "\" }\n"
       << indenter(indent + 1) << "is_error = { " << is_error << " }\n"
       << indenter(indent) << "}";

    return ss.str();
}

} // namespace MIR::IR
