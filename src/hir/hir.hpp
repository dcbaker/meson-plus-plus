// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <variant>

namespace HIR {

/**
 * A Generic holder type.
 *
 * This is used to hold assignments of a specific type and it's assignment
 * information
 */
template <typename T> class Holder {
  public:
    Holder(const T & v, const std::string & n) : value{v}, variable_name{n} {};
    Holder(const T & v) : value{v}, variable_name{std::nullopt} {};
    virtual ~Holder(){};

    const T value;
    const std::optional<std::string> variable_name;
    std::optional<std::uint16_t> value_number = std::nullopt;
};

using StringHolder = Holder<std::string>;
using NumberHolder = Holder<std::int64_t>;

using Holders = std::variant<StringHolder, NumberHolder>;

class Node {
  public:
    Node(const Holders & h) : holder{h} {};
    virtual ~Node(){};

    Holders holder;

    Node * prev = nullptr;
};

} // namespace HIR
