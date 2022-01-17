// SPDX-license-identifier: Apache-2.0
// Copyright © 2022 Dylan Baker

#include "exceptions.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

std::optional<Object> lower_found_method(const Object & obj) {
    if (!std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        return std::nullopt;
    }
    const auto & f = std::get<std::shared_ptr<FunctionCall>>(obj);

    if (!(f->holder.has_value() &&
          std::holds_alternative<std::shared_ptr<Program>>(f->holder.value())) ||
        f->name != "found") {
        return std::nullopt;
    }

    if (!f->pos_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Program.found() does not take any positional arguments");
    }
    if (!f->kw_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "Program.found() does not take any keyword arguments");
    }

    return std::make_shared<Boolean>(
        std::get<std::shared_ptr<Program>>(f->holder.value())->found());
}

} // namespace

bool lower_program_objects(BasicBlock & block, State::Persistant & pstate) {
    bool progress = false;
    // clang-format off
    return false
        || function_walker(&block, lower_found_method)
        ;
    // clang-format on

    return progress;
}

} // namespace MIR::Passes
