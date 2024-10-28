// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

/**
 * Helpers to extract arguments and keyword arguments
 */

#pragma once

#include "mir.hpp"

#include <optional>
#include <variant>

namespace MIR::Passes {

/// @brief Destructure an Instruction
/// @tparam T The type to excract
/// @param arg the Instruction to extract from
/// @return either the value if it is of that type, or a nullopt
template <typename T> std::optional<T> extract_positional_argument(const Instruction & arg) {
    if (std::holds_alternative<T>(*arg.obj_ptr)) {
        return std::get<T>(*arg.obj_ptr);
    }
    return std::nullopt;
}

template <typename R> R _extract_positional_argument_v(const Instruction & arg) {
    return std::monostate{};
}

template <typename R, typename T, typename... Args>
R _extract_positional_argument_v(const Instruction & arg) {
    if (std::holds_alternative<T>(*arg.obj_ptr)) {
        return std::get<T>(*arg.obj_ptr);
    }
    return _extract_positional_argument_v<R, Args...>(arg);
}

template <typename T, typename... Args>
std::variant<std::monostate, T, Args...> extract_positional_argument_v(const Instruction & arg) {
    if (std::holds_alternative<T>(*arg.obj_ptr)) {
        return std::get<T>(*arg.obj_ptr);
    }
    // TODO: this ignores invalid arguments
    return _extract_positional_argument_v<std::variant<std::monostate, T, Args...>, Args...>(arg);
}

/// @brief Extract a variadic number of arguments
/// @tparam T The type to extract
/// @param start The beging iterator to extract from
/// @param end the end iterator to extract from
/// @return A vector of values
template <typename T>
std::vector<T> extract_variadic_arguments(std::vector<Instruction>::const_iterator start,
                                          std::vector<Instruction>::const_iterator end) {
    std::vector<T> nobjs{};
    for (; start != end; start++) {
        if (std::holds_alternative<Array>(*start->obj_ptr)) {
            const auto & arr = std::get<Array>(*start->obj_ptr);
            const auto & nvals = extract_variadic_arguments<T>(arr.value.begin(), arr.value.end());
            nobjs.insert(nobjs.end(), nvals.begin(), nvals.end());
        } else {
            // TODO: this is going to ignore invalid arghuments
            std::optional<T> arg = extract_positional_argument<T>(*start->obj_ptr);
            if (arg) {
                nobjs.emplace_back(arg.value());
            }
        }
    }
    return nobjs;
}

/// @brief Extract a keyword argument from a mapping
/// @tparam T The type to extract
/// @param kwargs The mapping to extract from
/// @param name the name to get
/// @return the value associated with that name
template <typename T>
std::optional<T>
extract_keyword_argument(const std::unordered_map<std::string, Instruction> & kwargs,
                         const std::string & name) {
    const auto & found = kwargs.find(name);
    if (found == kwargs.end()) {
        return std::nullopt;
    }
    if (!std::holds_alternative<T>(*found->second.obj_ptr)) {
        // XXX: this is just going to ignore invalid arguments…
        return std::nullopt;
    }
    return std::get<T>(*found->second.obj_ptr);
}

/// @brief Extract a keyword argument that has a variant type
/// @tparam T The first variant type to extract
/// @tparam ...Args additional variant types
/// @param kwargs The mapping to extract from
/// @param name the name to get
/// @return the value associated with that name
template <typename T, typename... Args>
std::variant<std::monostate, T, Args...>
extract_keyword_argument_v(const std::unordered_map<std::string, Instruction> & kwargs,
                           const std::string & name) {
    // FIXME: this has the same problem as the other version, which is that you
    // can't distinguish between "not present" and "not a valid type"
    const auto & found = kwargs.find(name);
    if (found == kwargs.end()) {
        return std::monostate{};
    }
    return extract_positional_argument_v<T, Args...>(found->second);
}

/// @brief Extract a keyword argument that is an array of type
/// @tparam T The type to extract
/// @param kwargs The mapping to extract from
/// @param name the name to get
/// @return the value associated with that name
template <typename T>
std::vector<T>
extract_keyword_argument_a(const std::unordered_map<std::string, Instruction> & kwargs,
                           const std::string & name, const bool & as_list = false) {
    auto found = kwargs.find(name);
    if (found == kwargs.end()) {
        return {};
    }
    if (auto v = std::get_if<T>(found->second.obj_ptr.get())) {
        return {*v};
    }
    if (auto v = std::get_if<Array>(found->second.obj_ptr.get())) {
        std::vector<T> ret{};
        for (const auto & a : v->value) {
            auto arg = extract_positional_argument<T>(a);
            // XXX: also ignores invalid arguments
            if (arg) {
                ret.emplace_back(arg.value());
            }
        }
        return ret;
    }
    // XXX: This ignores invalid arguments
    return {};
}

/// @brief Extract a keyword argument that is an array of variant types
/// @tparam T The first variant type to extract
/// @tparam ...Args additional variant types
/// @param kwargs The mapping to extract from
/// @param name the name to get
/// @return the value associated with that name
template <typename... Args>
std::vector<std::variant<std::monostate, Args...>>
extract_keyword_argument_av(const std::unordered_map<std::string, Instruction> & kwargs,
                            const std::string & name) {
    // FIXME: this has the same problem as the other version, which is that you
    // can't distinguish between "not present" and "not a valid type"
    const auto & found = kwargs.find(name);
    if (found == kwargs.end()) {
        return {};
    }
    if (auto v = std::get_if<Array>(found->second.obj_ptr.get())) {
        std::vector<std::variant<std::monostate, Args...>> ret{};
        for (const auto & a : v->value) {
            // XXX: also ignores invalid arguments
            ret.emplace_back(extract_positional_argument_v<Args...>(a));
        }
        return ret;
    }
    return {extract_positional_argument_v<Args...>(found->second)};
}

} // namespace MIR::Passes
