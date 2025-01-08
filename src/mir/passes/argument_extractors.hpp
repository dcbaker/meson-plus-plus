// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

/**
 * Helpers to extract arguments and keyword arguments
 */

#pragma once

#include "exceptions.hpp"
#include "mir.hpp"

#include <optional>
#include <variant>

namespace MIR::Passes {

/// @brief Destructure an Object
/// @tparam T The type to extract
/// @param arg the Object to extract from
/// @return either the value if it is of that type, or a nullopt
template <typename T> std::optional<T> extract_positional_argument(const Object & obj) {
    if (std::holds_alternative<T>(obj)) {
        return std::get<T>(obj);
    }
    return std::nullopt;
}

/// @brief Extract a positional argument or fail
/// @tparam T The type to extract
/// @param arg the Object to extract from
/// @param err_msg The error message to throw if this fails
/// @return A copy of the instruction
template <typename T>
T extract_positional_argument(const Object & obj, const std::string & err_msg) {
    try {
        return std::get<T>(obj);
    } catch (const std::bad_variant_access &) {
        throw Util::Exceptions::InvalidArguments{err_msg};
    }
}

template <typename R> R _extract_positional_argument_v(const Object & obj) {
    return std::monostate{};
}

template <typename R, typename T, typename... Args>
R _extract_positional_argument_v(const Object & obj) {
    if (std::holds_alternative<T>(obj)) {
        return std::get<T>(obj);
    }
    return _extract_positional_argument_v<R, Args...>(obj);
}

/// @brief Extract a positional argument that is a variadic type
/// @tparam T The first type to try
/// @tparam ...Args additional types to try
/// @param arg The Object to extract from
/// @return The argument or a mononstate if the type is missing
template <typename T, typename... Args>
std::variant<std::monostate, T, Args...> extract_positional_argument_v(const Object & arg) {
    return _extract_positional_argument_v<std::variant<std::monostate, T, Args...>, T, Args...>(
        arg);
}

template <typename R>
R _extract_positional_argument_v(const Object & arg, const std::string & err_msg) {
    throw Util::Exceptions::InvalidArguments{err_msg};
}

template <typename R, typename T, typename... Args>
R _extract_positional_argument_v(const Object & arg, const std::string & err_msg) {
    if (std::holds_alternative<T>(arg)) {
        return std::get<T>(arg);
    }
    return _extract_positional_argument_v<R, Args...>(arg, err_msg);
}

/// @brief Extract a positional argument that is a variadic type
/// @tparam T The first type to try
/// @tparam ...Args additional types to try
/// @param arg The Object to extract from
/// @param err_msg a message to add to an exception if the variant cannot be found
/// @return The argument or a mononstate if the type is missing
template <typename T, typename... Args>
std::variant<T, Args...> extract_positional_argument_v(const Object & arg,
                                                       const std::string & err_msg) {
    return _extract_positional_argument_v<std::variant<T, Args...>, T, Args...>(arg, err_msg);
}

/// @brief Extract a variadic number of arguments
/// @tparam T The type to extract
/// @param start The beging iterator to extract from
/// @param end the end iterator to extract from
/// @return A vector of values
template <typename T>
std::vector<T> extract_variadic_arguments(std::vector<Object>::const_iterator start,
                                          std::vector<Object>::const_iterator end,
                                          const std::string & err_msg) {
    std::vector<T> nobjs{};
    for (; start != end; start++) {
        if (std::holds_alternative<ArrayPtr>(*start)) {
            const auto & arr = std::get<ArrayPtr>(*start);
            const auto & nvals =
                extract_variadic_arguments<T>(arr->value.begin(), arr->value.end(), err_msg);
            nobjs.insert(nobjs.end(), nvals.begin(), nvals.end());
        } else {
            nobjs.emplace_back(extract_positional_argument<T>(*start, err_msg));
        }
    }
    return nobjs;
}

/// @brief Extract a keyword argument from a mapping
/// @tparam T The type to extract
/// @param kwargs The mapping to extract from
/// @param name the name to get
/// @param err_msg The message to throw if the argument is of the incorrect type
/// @return the value associated with that name
template <typename T>
std::optional<T> extract_keyword_argument(const std::unordered_map<std::string, Object> & kwargs,
                                          const std::string & name, const std::string & err_msg) {
    const auto & found = kwargs.find(name);
    if (found == kwargs.end()) {
        return std::nullopt;
    }
    return extract_positional_argument<T>(found->second, err_msg);
}

/// @brief Extract a keyword argument that has a variant type
/// @tparam T The first variant type to extract
/// @tparam ...Args additional variant types
/// @param kwargs The mapping to extract from
/// @param name the name to get
/// @return the value associated with that name
template <typename T, typename... Args>
std::variant<std::monostate, T, Args...>
extract_keyword_argument_v(const std::unordered_map<std::string, Object> & kwargs,
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
/// @param err_msg The message to display if the type is incorrect
/// @return the value associated with that name
template <typename T>
std::optional<std::vector<T>>
extract_keyword_argument_a(const std::unordered_map<std::string, Object> & kwargs,
                           const std::string & name, const std::string & err_msg) {
    auto found = kwargs.find(name);
    if (found == kwargs.end()) {
        return std::nullopt;
    }
    if (auto v = std::get_if<T>(&found->second)) {
        return std::vector{*v};
    }
    if (std::holds_alternative<ArrayPtr>(found->second)) {
        auto v = std::get<ArrayPtr>(found->second);
        std::vector<T> ret{};
        for (const auto & a : v->value) {
            ret.emplace_back(extract_positional_argument<T>(a, err_msg));
        }
        return ret;
    }
    throw Util::Exceptions::InvalidArguments{err_msg};
}

/// @brief Extract a keyword argument that is an array of variant types
/// @tparam T The first variant type to extract
/// @tparam ...Args additional variant types
/// @param kwargs The mapping to extract from
/// @param name the name to get
/// @return the value associated with that name
template <typename... Args>
std::optional<std::vector<std::variant<Args...>>>
extract_keyword_argument_av(const std::unordered_map<std::string, Object> & kwargs,
                            const std::string & name, const std::string & err_msg) {
    const auto & found = kwargs.find(name);
    if (found == kwargs.end()) {
        return std::nullopt;
    }
    if (std::holds_alternative<ArrayPtr>(found->second)) {
        auto v = std::get<ArrayPtr>(found->second);
        std::vector<std::variant<Args...>> ret{};
        for (const auto & a : v->value) {
            // XXX: also ignores invalid arguments
            ret.emplace_back(extract_positional_argument_v<Args...>(a, err_msg));
        }
        return ret;
    }
    return std::vector<std::variant<Args...>>{
        extract_positional_argument_v<Args...>(found->second, err_msg)};
}

} // namespace MIR::Passes
