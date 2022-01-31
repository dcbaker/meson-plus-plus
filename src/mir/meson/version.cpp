// SPDX-license-identifier: Apache-2.0
// Copyright © 2022 Intel Corporation

#include <tuple>
#include <vector>

#include "version.hpp"

namespace MIR::Version {

namespace {

/// Split values into alpha or digit groups
std::vector<std::tuple<std::string, bool>> split_values(const std::string & str) {
    std::vector<std::tuple<std::string, bool>> out{};
    std::string current{};
    bool last_is_digit = true;

    for (const auto & s : str) {
        if (!std::isalnum(s) || std::isdigit(s) != last_is_digit) {
            if (!current.empty()) {
                out.emplace_back(std::make_tuple(current, last_is_digit));
                last_is_digit = true;
                current = {};
            }
        }
        if (std::isalnum(s)) {
            last_is_digit &= std::isdigit(s);
            current.insert(current.end(), s);
        }
    }
    // Put any remaining values in the returned value
    out.emplace_back(std::make_tuple(current, last_is_digit));

    return out;
}

template <typename T> bool compare_impl(const T & in1, const Operator & op, const T & in2) {
    switch (op) {
        case Operator::LT:
            return in1 < in2;
        case Operator::LE:
            return in1 <= in2;
        case Operator::EQ:
            return in1 == in2;
        case Operator::NE:
            return in1 != in2;
        case Operator::GE:
            return in1 >= in2;
        case Operator::GT:
            return in1 > in2;
    }
    throw std::exception{}; // Should be unreachable
}

} // namespace

std::string to_string(const Operator & op) {
    switch (op) {
        case Operator::LT:
            return "<";
        case Operator::LE:
            return "<=";
        case Operator::EQ:
            return "==";
        case Operator::NE:
            return "!=";
        case Operator::GE:
            return ">=";
        case Operator::GT:
            return ">";
    }
    throw std::exception{}; // Should be unreachable
}

bool compare(const std::string & v1, const Operator & op, const std::string & v2) {
    // Drop any non-alphanumeric characters, those are not compared
    const std::vector<std::tuple<std::string, bool>> str1 = split_values(v1);
    const std::vector<std::tuple<std::string, bool>> str2 = split_values(v2);

    auto itr1 = str1.begin();
    auto itr2 = str2.begin();

    for (; itr1 != str1.end() && itr2 != str2.end(); ++itr1, ++itr2) {
        const auto & [v1, ours_is_digit] = *itr1;
        const auto & [v2, theirs_is_digit] = *itr2;
        if (ours_is_digit != theirs_is_digit) {
            return compare_impl(ours_is_digit, op, theirs_is_digit);
        }

        // We know both are, or aren't
        if (ours_is_digit) {
            int i1 = std::atoi(v1.c_str());
            int i2 = std::atoi(v2.c_str());

            if (i1 != i2) {
                return compare_impl(i1, op, i2);
            }
        } else if (v1 != v2) {
            return compare_impl(v1, op, v2);
        }
    }

    return compare_impl(itr1 != str1.end(), op, itr2 != str2.end());
}

} // namespace MIR::Version
