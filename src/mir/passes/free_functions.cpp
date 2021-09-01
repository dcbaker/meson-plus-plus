// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include <vector>

#include "exceptions.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

// XXX: we probably need access to the source_root and build_root
std::optional<Object> lower_files(const Object & obj, const State::Persistant & pstate) {
    if (!std::holds_alternative<std::unique_ptr<FunctionCall>>(obj)) {
        return std::nullopt;
    }
    const auto & f = std::get<std::unique_ptr<FunctionCall>>(obj);

    // XXX: I think this can happen if a replacement happens, but I also think
    // it's a bug
    if (f.get() == nullptr) {
        return std::nullopt;
    }

    if (f->holder.value_or("") != "" || f->name != "files") {
        return std::nullopt;
    }

    std::vector<Object> files{};
    for (const auto & arg_h : f->pos_args) {
        // XXX: do something more realistic here
        // This could be Array<STring> and still be valid.
        if (!std::holds_alternative<std::unique_ptr<String>>(arg_h)) {
            throw Util::Exceptions::InvalidArguments("Arguments to 'files()' must be strings");
        }
        auto const & v = std::get<std::unique_ptr<String>>(arg_h);

        files.emplace_back(std::make_unique<File>(
            Objects::File{v->value, false, pstate.source_root, pstate.build_root}));
    }

    return std::make_unique<Array>(std::move(files));
}

} // namespace

bool lower_free_functions(BasicBlock * block, const State::Persistant & pstate) {
    bool progress = false;

    progress |=
        function_walker(block, [&](const Object & obj) { return lower_files(obj, pstate); });

    return progress;
}

} // namespace MIR::Passes
