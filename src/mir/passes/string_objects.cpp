// SPDX-license-identifier: Apache-2.0
// Copyright © 2022 Dylan Baker

#include "argument_extractors.hpp"
#include "exceptions.hpp"
#include "meson/version.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

std::optional<Object> lower_version_compare_method(const FunctionCall & f) {
    if (!f.kw_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "string.version_compare() does not take any keyword arguments");
    }

    if (f.pos_args.size() != 1) {
        throw Util::Exceptions::InvalidArguments(
            "string.version_compare() takes exactly 1 positional argument, got: " +
            std::to_string(f.pos_args.size()));
    }

    // XXX: really need to check that this has a value...
    const auto & c = *extract_positional_argument<std::shared_ptr<String>>(f.pos_args[0]).value();
    const auto & s = *std::get<std::shared_ptr<String>>(f.holder.value());

    std::string cval{};
    for (const auto & ch : c.value) {
        if (!std::isspace(ch)) {
            cval.insert(cval.end(), ch);
        }
    }

    Version::Operator op;
    std::string val;
    if (cval.substr(0, 2) == "==") {
        op = Version::Operator::EQ;
        val = cval.substr(2, cval.size());
    } else if (cval.substr(0, 2) == "!=") {
        op = Version::Operator::NE;
        val = cval.substr(2, cval.size());
    } else if (cval.substr(0, 2) == ">=") {
        op = Version::Operator::GE;
        val = cval.substr(2, cval.size());
    } else if (cval.substr(0, 2) == "<=") {
        op = Version::Operator::LE;
        val = cval.substr(2, cval.size());
    } else if (cval.substr(0, 1) == "<") {
        op = Version::Operator::LT;
        val = cval.substr(1, cval.size());
    } else if (cval.substr(0, 1) == ">") {
        op = Version::Operator::GT;
        val = cval.substr(1, cval.size());
    } else {
        throw Util::Exceptions::MesonException(
            "Version string comparison does not start with a valid comparison operator: " +
            c.value);
    }

    return std::make_shared<Boolean>(Version::compare(s.value, op, val));
}

std::optional<Object> lower_string_methods_impl(const Object & obj,
                                                const State::Persistant & pstate) {
    if (!std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        return std::nullopt;
    }
    const auto & f = *std::get<std::shared_ptr<FunctionCall>>(obj);

    if (!(f.holder.has_value() &&
          std::holds_alternative<std::shared_ptr<String>>(f.holder.value()))) {
        return std::nullopt;
    }

    if (!all_args_reduced(f.pos_args, f.kw_args)) {
        return std::nullopt;
    }

    if (f.name == "version_compare") {
        return lower_version_compare_method(f);
    }

    // XXX: Shouldn't really be able to get here...
    return std::nullopt;
}

} // namespace

bool lower_string_objects(BasicBlock & block, State::Persistant & pstate) {
    return function_walker(
        block, [&](const Object & obj) { return lower_string_methods_impl(obj, pstate); });
}

} // namespace MIR::Passes
