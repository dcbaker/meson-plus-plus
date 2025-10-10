// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#include "mir.hpp"

#include <sstream>

namespace MIR {

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

Message::Message(MessageType t, std::string s)
    : type{t}, message{std::move(s)}, is_error{t == MessageType::error} {};
Message::Message(MessageType t, std::string s, bool e)
    : type{t}, message{std::move(s)}, is_error{e} {};

std::string Message::serialize() const {
    std::stringstream ss{};
    ss << "Message { "
       << "type = { " << to_string(type) << " } "
       << "message = { '" << message << "' } "
       << "is_error = { " << is_error << " } "
       << "}";

    return ss.str();
}

} // namespace MIR
