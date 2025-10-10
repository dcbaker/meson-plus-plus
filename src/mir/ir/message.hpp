// SPDX-License-Identifier: Apache-2.0
// Copyright © 2025 Intel Corporation

#pragma once

#include <string>

namespace MIR::IR {

/// @brief What kind of message is this.
enum class MessageType {
    /// @brief A debug only message
    debug,

    /// @brief A standard message
    message,

    /// @brief A warning
    warning,

    /// @brief An error that has occurred
    error,
};

/// @brief A printed message of some kind
class Message {
  public:
    Message(MessageType t, std::string s);
    Message(MessageType t, std::string s, bool e);

    /// @brief provide a serialized form of this instruction
    std::string serialize() const;

    /// @brief What type of message is this
    MessageType type;

    /// @brief The message itself
    std::string message;

    /// @brief Is this an error?
    bool is_error;
};

} // namespace MIR::IR
