// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include <iostream>
#include <vector>

#include "exceptions.hpp"
#include "log.hpp"
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

/**
 * Convert source to file
 *
 * This is only for Files, not for targets. I want to separate targets into a
 * separate structure (or multiple structures, probably).
 *
 * This walks a vector of Objects, creating a new, flat vector of Files,
 * converting any strings into files, appending files as is, and flattening any
 * arrays it runs into.
 */
std::vector<Objects::File> srclist_to_filelist(const std::vector<Object> & srclist,
                                               const State::Persistant & pstate) {
    std::vector<Objects::File> filelist{};
    for (const auto & s : srclist) {
        if (std::holds_alternative<std::unique_ptr<String>>(s)) {
            const auto & src = std::get<std::unique_ptr<String>>(s);
            filelist.emplace_back(
                Objects::File{src->value, false, pstate.source_root, pstate.build_root});
        } else if (std::holds_alternative<std::unique_ptr<File>>(s)) {
            filelist.emplace_back(std::get<std::unique_ptr<File>>(s)->file);
        } else if (std::holds_alternative<std::unique_ptr<Array>>(s)) {
            auto a = srclist_to_filelist(std::get<std::unique_ptr<Array>>(s)->value, pstate);
            std::move(a.begin(), a.end(), std::back_inserter(filelist));
        } else {
            // TODO: there are other valid types here, like generator output and custom targets
            throw Util::Exceptions::InvalidArguments{
                "'executable' sources must be strings or files."};
        }
    }

    return filelist;
}

std::optional<Object> lower_executable(const Object & obj, const State::Persistant & pstate) {
    if (!std::holds_alternative<std::unique_ptr<FunctionCall>>(obj)) {
        return std::nullopt;
    }
    const auto & f = std::get<std::unique_ptr<FunctionCall>>(obj);

    if (f->holder.value_or("") != "" || f->name != "executable") {
        return std::nullopt;
    }

    // This doesn't handle the listified version corretly
    if (f->pos_args.size() < 2) {
        throw Util::Exceptions::InvalidArguments{"executable requires at least 2 arguments"};
    }
    if (!std::holds_alternative<std::unique_ptr<String>>(f->pos_args[0])) {
        // TODO: it could also be an identifier pointing to a string
        throw Util::Exceptions::InvalidArguments{"executable first argument must be a string"};
    }
    const auto & name = std::get<std::unique_ptr<String>>(f->pos_args[0])->value;

    // skip the first argument
    // XXX: I don't like mutating pos_args here, but it's working for the moment
    // and it's easy
    std::vector<Object> raw_srcs{};
    std::move(f->pos_args.begin() + 1, f->pos_args.end(), std::back_inserter(raw_srcs));
    auto srcs = srclist_to_filelist(raw_srcs, pstate);

    Objects::Executable exe{name, {}};

    return std::make_unique<Executable>(exe);
}

} // namespace

void lower_project(BasicBlock * block, State::Persistant & pstate) {
    const auto & obj = block->instructions.front();

    if (!std::holds_alternative<std::unique_ptr<FunctionCall>>(obj)) {
        throw Util::Exceptions::MesonException{
            "First non-whitespace, non-comment must be a call to project()"};
    }
    const auto & f = std::get<std::unique_ptr<FunctionCall>>(obj);

    if (f->name != "project") {
        throw Util::Exceptions::MesonException{
            "First non-whitespace, non-comment must be a call to project()"};
    }

    // This doesn't handle the listified version corretly
    if (f->pos_args.size() < 1) {
        throw Util::Exceptions::InvalidArguments{"project requires at least 1 argument"};
    }
    if (!std::holds_alternative<std::unique_ptr<String>>(f->pos_args[0])) {
        // TODO: it could also be an identifier pointing to a string
        throw Util::Exceptions::InvalidArguments{"project first argument must be a string"};
    }
    pstate.name = std::get<std::unique_ptr<String>>(f->pos_args[0])->value;
    std::cout << "Project name: " << Util::Log::bold(pstate.name) << std::endl;

    // Remove the valid project() call so we don't accidently find it later when
    // looking for invalid function calls.
    block->instructions.pop_front();
}

bool lower_free_functions(BasicBlock * block, const State::Persistant & pstate) {
    bool progress = false;

    progress |=
        function_walker(block, [&](const Object & obj) { return lower_files(obj, pstate); });
    progress |=
        function_walker(block, [&](const Object & obj) { return lower_executable(obj, pstate); });

    return progress;
}

} // namespace MIR::Passes
