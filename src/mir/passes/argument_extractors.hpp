// SPDX-license-identifier: Apache-2.0
// Copyright © 2021-2022 Dylan Baker

/**
 * Helpers to extract arguments and keyword arguments
 */

#pragma once

#include <optional>
#include <variant>

#include "mir.hpp"

namespace MIR::Passes {

template <typename T> std::optional<T> extract_positional_argument(const Object & arg) {
    if (std::holds_alternative<T>(arg)) {
        return std::get<T>(arg);
    }
    // TODO: this ignores invalid arguments
    return std::nullopt;
}

template <typename R> R _extract_positional_argument_v(const Object & arg) {
    return std::monostate{};
}

template <typename R, typename T, typename... Args>
R _extract_positional_argument_v(const Object & arg) {
    if (std::holds_alternative<T>(arg)) {
        return std::get<T>(arg);
    }
    return _extract_positional_argument_v<R, Args...>(arg);
}

template <typename T, typename... Args>
std::variant<std::monostate, T, Args...> extract_positional_argument_v(const Object & arg) {
    if (std::holds_alternative<T>(arg)) {
        return std::get<T>(arg);
    }
    // TODO: this ignores invalid arguments
    return _extract_positional_argument_v<std::variant<std::monostate, T, Args...>, Args...>(arg);
}

template <typename T>
std::vector<T> extract_variadic_arguments(std::vector<Object>::const_iterator start,
                                          std::vector<Object>::const_iterator end) {
    std::vector<T> nobjs{};
    for (; start != end; start++) {
        if (std::holds_alternative<std::shared_ptr<Array>>(*start)) {
            const auto & arr = *std::get<std::shared_ptr<Array>>(*start);
            const auto & nvals = extract_variadic_arguments<T>(arr.value.begin(), arr.value.end());
            nobjs.insert(nobjs.end(), nvals.begin(), nvals.end());
        } else {
            // TODO: this is going to ignore invalid arghuments
            std::optional<T> arg = extract_positional_argument<T>(*start);
            if (arg) {
                nobjs.emplace_back(arg.value());
            }
        }
    }
    return nobjs;
}

template <typename T>
std::optional<T> extract_keyword_argument(const std::unordered_map<std::string, Object> & kwargs,
                                          const std::string & name) {
    const auto & found = kwargs.find(name);
    if (found == kwargs.end()) {
        return std::nullopt;
    } else if (!std::holds_alternative<T>(found->second)) {
        // XXX: this is just going to ignore invalid arguments…
        return std::nullopt;
    }
    return std::get<T>(found->second);
}

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

template <typename T>
std::vector<T> extract_keyword_argument_a(const std::unordered_map<std::string, Object> & kwargs,
                                          const std::string & name, const bool & as_list = false) {
    auto found = kwargs.find(name);
    if (found == kwargs.end()) {
        return {};
    } else if (std::holds_alternative<T>(found->second)) {
        return {std::vector<T>{std::get<T>(found->second)}};
    } else if (std::holds_alternative<std::shared_ptr<Array>>(found->second)) {
        std::vector<T> ret{};
        for (const auto & a : std::get<std::shared_ptr<Array>>(found->second)->value) {
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

template <typename... Args>
std::vector<std::variant<std::monostate, Args...>>
extract_keyword_argument_av(const std::unordered_map<std::string, Object> & kwargs,
                            const std::string & name) {
    // FIXME: this has the same problem as the other version, which is that you
    // can't distinguish between "not present" and "not a valid type"
    const auto & found = kwargs.find(name);
    if (found == kwargs.end()) {
        return {};
    } else if (std::holds_alternative<std::shared_ptr<Array>>(found->second)) {
        std::vector<std::variant<std::monostate, Args...>> ret{};
        for (const auto & a : std::get<std::shared_ptr<Array>>(found->second)->value) {
            // XXX: also ignores invalid arguments
            ret.emplace_back(extract_positional_argument_v<Args...>(a));
        }
        return ret;
    } else {
        return {extract_positional_argument_v<Args...>(found->second)};
    }
}

} // namespace MIR::Passes
