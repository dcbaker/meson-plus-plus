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
    if (!std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        return std::nullopt;
    }
    const auto & f = std::get<std::shared_ptr<FunctionCall>>(obj);

    if (f->holder.value_or("") != "" || f->name != "files") {
        return std::nullopt;
    }

    std::vector<Object> files{};
    for (const auto & arg_h : f->pos_args) {
        // XXX: do something more realistic here
        // This could be Array<STring> and still be valid.
        if (!std::holds_alternative<std::shared_ptr<String>>(arg_h)) {
            throw Util::Exceptions::InvalidArguments("Arguments to 'files()' must be strings");
        }
        auto const & v = std::get<std::shared_ptr<String>>(arg_h);

        files.emplace_back(std::make_shared<File>(
            Objects::File{v->value, f->source_dir, false, pstate.source_root, pstate.build_root}));
    }

    return std::make_shared<Array>(std::move(files));
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
std::vector<Objects::File> srclist_to_filelist(const std::vector<Object *> & srclist,
                                               const State::Persistant & pstate,
                                               const std::string & subdir) {
    std::vector<Objects::File> filelist{};
    for (const auto & s : srclist) {
        if (const auto src = std::get_if<std::shared_ptr<String>>(s); src != nullptr) {
            filelist.emplace_back(
                Objects::File{(*src)->value, subdir, false, pstate.source_root, pstate.build_root});
        } else if (const auto src = std::get_if<std::shared_ptr<File>>(s); s != nullptr) {
            filelist.emplace_back((*src)->file);
        } else {
            // TODO: there are other valid types here, like generator output and custom targets
            throw Util::Exceptions::InvalidArguments{
                "'executable' sources must be strings or files."};
        }
    }

    return filelist;
}

std::unordered_map<Toolchain::Language, std::vector<Arguments::Argument>>
target_arguments(const std::shared_ptr<FunctionCall> & f, const State::Persistant & pstate) {
    std::unordered_map<Toolchain::Language, std::vector<Arguments::Argument>> args{};

    // TODO: handle more than just cpp, likely using a loop
    if (f->kw_args.find("cpp_args") != f->kw_args.end()) {
        const auto & args_obj = f->kw_args["cpp_args"];
        const auto & comp = pstate.toolchains.at(Toolchain::Language::CPP).build()->compiler;
        if (std::holds_alternative<std::shared_ptr<String>>(args_obj)) {
            const auto & v = std::get<std::shared_ptr<String>>(args_obj)->value;
            args[Toolchain::Language::CPP] =
                std::vector<Arguments::Argument>{comp->generalize_argument(v)};
        } else if (std::holds_alternative<std::shared_ptr<Array>>(args_obj)) {
            std::vector<Arguments::Argument> cpp_args{};
            const auto & raw_args = std::get<std::shared_ptr<Array>>(args_obj)->value;
            for (const auto & ra : raw_args) {
                if (!std::holds_alternative<std::shared_ptr<String>>(ra)) {
                    throw Util::Exceptions::MesonException{"\"cpp_args\" must be strings"};
                }
                // TODO need to lower this
                const auto & a = std::get<std::shared_ptr<String>>(ra)->value;
                cpp_args.emplace_back(comp->generalize_argument(a));
            }

            args[Toolchain::Language::CPP] = std::move(cpp_args);
        } else {
            throw Util::Exceptions::InvalidArguments{
                "executable cpp_args must be an array of strings"};
        }
    }

    return args;
}

std::vector<Objects::StaticLinkage>
target_kwargs(const std::unordered_map<std::string, Object> & kwargs) {
    std::vector<Objects::StaticLinkage> s_link{};
    if (const auto & raw_slink = kwargs.find("link_with"); raw_slink != kwargs.end()) {
        if (std::holds_alternative<std::shared_ptr<StaticLibrary>>(raw_slink->second)) {
            s_link.emplace_back(Objects::StaticLinkage{
                Objects::StaticLinkMode::NORMAL,
                &std::get<std::shared_ptr<StaticLibrary>>(raw_slink->second)->value});
        } else if (std::holds_alternative<std::shared_ptr<Array>>(raw_slink->second)) {
            const auto & list = std::get<std::shared_ptr<Array>>(raw_slink->second)->value;
            for (const auto & a : list) {
                if (std::get<std::shared_ptr<StaticLibrary>>(a)) {
                    s_link.emplace_back(Objects::StaticLinkage{
                        Objects::StaticLinkMode::NORMAL,
                        &std::get<std::shared_ptr<StaticLibrary>>(a)->value});
                } else {
                    // TODO: real error here
                    throw std::exception{};
                }
            }
        } else {
            // TODO: real error here
            throw std::exception{};
        }
    }

    return s_link;
}

bool all_args_reduced(const std::vector<Object> & pos_args,
                      const std::unordered_map<std::string, Object> & kw_args) {
    for (const auto & p : pos_args) {
        if (std::holds_alternative<std::unique_ptr<Identifier>>(p)) {
            return false;
        } else if (std::holds_alternative<std::shared_ptr<Array>>(p)) {
            for (const auto & a : std::get<std::shared_ptr<Array>>(p)->value) {
                if (std::holds_alternative<std::unique_ptr<Identifier>>(a)) {
                    return false;
                }
            }
        }
    }
    for (const auto & [_, p] : kw_args) {
        if (std::holds_alternative<std::unique_ptr<Identifier>>(p)) {
            return false;
        } else if (std::holds_alternative<std::shared_ptr<Array>>(p)) {
            for (const auto & a : std::get<std::shared_ptr<Array>>(p)->value) {
                if (std::holds_alternative<std::unique_ptr<Identifier>>(a)) {
                    return false;
                }
            }
        }
    }
    return true;
}

std::optional<Object> lower_executable(const Object & obj, const State::Persistant & pstate) {
    if (!std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        return std::nullopt;
    }
    const auto & f = std::get<std::shared_ptr<FunctionCall>>(obj);

    if (f->holder.value_or("") != "" || f->name != "executable") {
        return std::nullopt;
    } else if (!all_args_reduced(f->pos_args, f->kw_args)) {
        return std::nullopt;
    }

    // This doesn't handle the listified version corretly
    if (f->pos_args.size() < 2) {
        throw Util::Exceptions::InvalidArguments{"executable requires at least 2 arguments"};
    }

    if (!std::holds_alternative<std::shared_ptr<String>>(f->pos_args[0])) {
        // TODO: it could also be an identifier pointing to a string
        throw Util::Exceptions::InvalidArguments{"executable first argument must be a string"};
    }
    const auto & name = std::get<std::shared_ptr<String>>(f->pos_args[0])->value;

    // skip the first argument
    std::vector<Object *> raw_srcs{};
    for (unsigned i = 1; i < f->pos_args.size(); ++i) {
        raw_srcs.emplace_back(&f->pos_args[i]);
    }
    auto srcs = srclist_to_filelist(raw_srcs, pstate, f->source_dir);
    auto args = target_arguments(f, pstate);
    auto slink = target_kwargs(f->kw_args);

    // TODO: machien parameter needs to be set from the native kwarg
    Objects::Executable exe{name, srcs, Machines::Machine::BUILD, f->source_dir, args, slink};

    return std::make_shared<Executable>(exe, f->var);
}

std::optional<Object> lower_static_library(const Object & obj, const State::Persistant & pstate) {
    if (!std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        return std::nullopt;
    }
    const auto & f = std::get<std::shared_ptr<FunctionCall>>(obj);

    if (f->holder.value_or("") != "" || f->name != "static_library") {
        return std::nullopt;
    } else if (!all_args_reduced(f->pos_args, f->kw_args)) {
        return std::nullopt;
    }

    // This doesn't handle the listified version corretly
    if (f->pos_args.size() < 2) {
        throw Util::Exceptions::InvalidArguments{"static_library requires at least 2 arguments"};
    }
    if (!std::holds_alternative<std::shared_ptr<String>>(f->pos_args[0])) {
        // TODO: it could also be an identifier pointing to a string
        throw Util::Exceptions::InvalidArguments{"static_library first argument must be a string"};
    }
    const auto & name = std::get<std::shared_ptr<String>>(f->pos_args[0])->value;

    // skip the first argument
    std::vector<Object *> raw_srcs{};
    for (unsigned i = 1; i < f->pos_args.size(); ++i) {
        raw_srcs.emplace_back(&f->pos_args[i]);
    }
    auto srcs = srclist_to_filelist(raw_srcs, pstate, f->source_dir);
    auto args = target_arguments(f, pstate);
    auto slink = target_kwargs(f->kw_args);

    // TODO: machien parameter needs to be set from the native kwarg
    Objects::StaticLibrary lib{name, srcs, Machines::Machine::BUILD, f->source_dir, args, slink};

    return std::make_shared<StaticLibrary>(lib, f->var);
}

} // namespace

void lower_project(BasicBlock * block, State::Persistant & pstate) {
    const auto & obj = block->instructions.front();

    if (!std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        throw Util::Exceptions::MesonException{
            "First non-whitespace, non-comment must be a call to project()"};
    }
    const auto & f = std::get<std::shared_ptr<FunctionCall>>(obj);

    if (f->name != "project") {
        throw Util::Exceptions::MesonException{
            "First non-whitespace, non-comment must be a call to project()"};
    }

    // This doesn't handle the listified version corretly
    if (f->pos_args.size() < 1) {
        throw Util::Exceptions::InvalidArguments{"project requires at least 1 argument"};
    }
    if (!std::holds_alternative<std::shared_ptr<String>>(f->pos_args[0])) {
        // TODO: it could also be an identifier pointing to a string
        throw Util::Exceptions::InvalidArguments{"project first argument must be a string"};
    }
    pstate.name = std::get<std::shared_ptr<String>>(f->pos_args[0])->value;
    std::cout << "Project name: " << Util::Log::bold(pstate.name) << std::endl;

    // The rest of the poisitional arguments are languages
    // TODO: and these could be passed as a list as well.
    for (auto it = f->pos_args.begin() + 1; it != f->pos_args.end(); ++it) {
        if (!std::holds_alternative<std::shared_ptr<String>>(*it)) {
            throw Util::Exceptions::MesonException{
                "All additional arguments to project must be strings"};
        }
        const auto & f = std::get<std::shared_ptr<String>>(*it);
        const auto l = Toolchain::from_string(f->value);

        auto & tc = pstate.toolchains[l];

        // TODO: need to do host as well, when that is relavent
        tc.set(Machines::Machine::BUILD,
               std::make_shared<Toolchain::Toolchain>(
                   Toolchain::get_toolchain(l, Machines::Machine::BUILD)));
        const auto & c = tc.build()->compiler;

        // TODO: print the print the full version
        std::cout << c->language()
                  << " compiler for the for build machine: " << Util::Log::bold(c->id()) << " ("
                  << ")" << std::endl;

        const auto & lnk = tc.build()->linker;

        // TODO: print the print the full version
        std::cout << c->language()
                  << " linker for the for build machine: " << Util::Log::bold(lnk->id()) << " ("
                  << ")" << std::endl;
    }

    // TODO: handle keyword arguments

    // Remove the valid project() call so we don't accidently find it later when
    // looking for invalid function calls.
    block->instructions.pop_front();
}

bool lower_free_functions(BasicBlock * block, const State::Persistant & pstate) {
    // clang-format off
    return false
        || function_walker(block, [&](const Object & obj) { return lower_files(obj, pstate); })
        || function_walker(block, [&](const Object & obj) { return lower_executable(obj, pstate); })
        || function_walker(block, [&](const Object & obj) { return lower_static_library(obj, pstate); })
        ;
    // clang-format on
}

} // namespace MIR::Passes
