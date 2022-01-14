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

inline std::unordered_map<Toolchain::Language, std::vector<Arguments::Argument>>
target_arguments(const std::shared_ptr<FunctionCall> & f, const State::Persistant & pstate) {
    std::unordered_map<Toolchain::Language, std::vector<Arguments::Argument>> args{};

    const auto & comp = pstate.toolchains.at(Toolchain::Language::CPP).build()->compiler;
    auto raw_args = extract_array_keyword_argument<std::shared_ptr<String>>(f->kw_args, "cpp_args");
    for (const auto & ra : raw_args) {
        args[Toolchain::Language::CPP].emplace_back(comp->generalize_argument(ra->value));
    }

    return args;
}

inline std::vector<Objects::StaticLinkage>
target_kwargs(const std::unordered_map<std::string, Object> & kwargs) {
    std::vector<Objects::StaticLinkage> s_link{};
    auto raw_args =
        extract_array_keyword_argument<std::shared_ptr<StaticLibrary>>(kwargs, "link_with");
    for (const auto & s : raw_args) {
        s_link.emplace_back(Objects::StaticLinkage{Objects::StaticLinkMode::NORMAL, &s->value});
    }

    return s_link;
}

template <typename T>
std::optional<std::shared_ptr<T>> lower_build_target(const Object & obj,
                                                     const State::Persistant & pstate) {
    if (!std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        return std::nullopt;
    }
    const auto & f = std::get<std::shared_ptr<FunctionCall>>(obj);

    std::string f_name;
    if constexpr (std::is_same<T, Executable>::value) {
        f_name = "executable";
    } else if constexpr (std::is_same<T, StaticLibrary>::value) {
        f_name = "static_library";
    } else {
        assert(false);
    }

    if (f->holder.value_or("") != "" || f->name != f_name) {
        return std::nullopt;
    } else if (!all_args_reduced(f->pos_args, f->kw_args)) {
        return std::nullopt;
    }

    // This doesn't handle the listified version corretly
    if (f->pos_args.size() < 2) {
        throw Util::Exceptions::InvalidArguments{f->name + " requires at least 2 arguments"};
    }

    if (!std::holds_alternative<std::shared_ptr<String>>(f->pos_args[0])) {
        // TODO: it could also be an identifier pointing to a string
        throw Util::Exceptions::InvalidArguments{f->name + " first argument must be a string"};
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
    auto raw_inc = extract_array_keyword_argument<std::shared_ptr<IncludeDirectories>>(
        f->kw_args, "include_directories", true);
    std::vector<Objects::IncludeDirectories> inc{};
    for (const auto & i : raw_inc) {
        inc.emplace_back(i->value);
    }

    // TODO: machien parameter needs to be set from the native kwarg
    if constexpr (std::is_same<T, Executable>::value) {
        Objects::Executable held{name,  srcs, Machines::Machine::BUILD, f->source_dir, args,
                                 slink, inc};
        return std::make_shared<T>(held, f->var);
    } else if constexpr (std::is_same<T, StaticLibrary>::value) {
        Objects::StaticLibrary held{name,  srcs, Machines::Machine::BUILD, f->source_dir, args,
                                    slink, inc};
        return std::make_shared<T>(held, f->var);
    } else {
        assert(false);
    }
}

std::optional<Object> lower_executable(const Object & obj, const State::Persistant & pstate) {
    return lower_build_target<Executable>(obj, pstate);
}

std::optional<Object> lower_static_library(const Object & obj, const State::Persistant & pstate) {
    return lower_build_target<StaticLibrary>(obj, pstate);
}

std::optional<Object> lower_include_dirs(const Object & obj, const State::Persistant & pstate) {
    if (!std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        return std::nullopt;
    }
    const auto & f = std::get<std::shared_ptr<FunctionCall>>(obj);

    if (f->holder.value_or("") != "" || f->name != "include_directories") {
        return std::nullopt;
    } else if (!all_args_reduced(f->pos_args, f->kw_args)) {
        return std::nullopt;
    }

    for (const auto & a : f->pos_args) {
        if (!std::holds_alternative<std::shared_ptr<String>>(a)) {
            throw Util::Exceptions::InvalidArguments{
                "include_directories: all positional arguments must be strings"};
        }
    }

    std::vector<std::string> dirs{};
    for (const auto & a : f->pos_args) {
        dirs.emplace_back(std::get<std::shared_ptr<String>>(a)->value);
    }

    auto is_system = extract_keyword_argument<std::shared_ptr<Boolean>>(f->kw_args, "is_system")
                         .value_or(std::make_shared<Boolean>(false));

    Objects::IncludeDirectories incs{dirs, is_system->value};

    return std::make_shared<IncludeDirectories>(incs);
}

std::optional<Object> lower_messages(const Object & obj) {
    if (!std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        return std::nullopt;
    }
    const auto & f = std::get<std::shared_ptr<FunctionCall>>(obj);

    if (f->holder.value_or("") != "" ||
        (f->name != "message" && f->name != "warning" && f->name != "error")) {
        return std::nullopt;
    } else if (!all_args_reduced(f->pos_args, f->kw_args)) {
        return std::nullopt;
    }

    MessageLevel level;
    if (f->name == "message") {
        level = MessageLevel::MESSAGE;
    } else if (f->name == "warning") {
        level = MessageLevel::WARN;
    } else if (f->name == "error") {
        level = MessageLevel::ERROR;
    }

    // TODO: Meson accepts anything as a message bascially, without flattening.
    // Currently, Meson++ flattens everything so I'm only going to allow strings for the moment.
    auto args =
        extract_variadic_arguments<std::shared_ptr<String>>(f->pos_args.begin(), f->pos_args.end());

    std::string message{};
    for (const auto & a : args) {
        if (!message.empty()) {
            message.append(" ");
        }
        message.append(a->value);
    }

    return std::make_unique<Message>(level, message);
}

std::optional<Object> lower_assert(const Object & obj) {
    if (!std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        return std::nullopt;
    }
    const auto & f = std::get<std::shared_ptr<FunctionCall>>(obj);

    if (f->holder.value_or("") != "" || f->name != "assert") {
        return std::nullopt;
    } else if (!all_args_reduced(f->pos_args, f->kw_args)) {
        return std::nullopt;
    }

    if (f->pos_args.size() < 1 || f->pos_args.size() > 2) {
        throw Util::Exceptions::InvalidArguments("assert: takes 1 or 2 arguments, got " +
                                                 f->pos_args.size());
    }

    const auto & value = extract_positional_argument<std::shared_ptr<Boolean>>(f->pos_args[0]);

    if (value.value()) {
        // TODO: maye have an assert level of message?
        // TODO, how to get the original values of this?
        const auto & message = extract_positional_argument<std::shared_ptr<String>>(f->pos_args[1]);
        return std::make_unique<Message>(MessageLevel::ERROR,
                                         "Assertion failed: " +
                                             message.value_or(std::make_shared<String>(""))->value);
    }

    // TODO: it would be better to
    return std::make_unique<Empty>();
}

bool holds_reduced(const Object & obj);

bool holds_reduced_array(const Object & obj) {
    if (std::holds_alternative<std::shared_ptr<Array>>(obj)) {
        for (const auto & a : std::get<std::shared_ptr<Array>>(obj)->value) {
            if (!holds_reduced(a)) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool holds_reduced_dict(const Object & obj) {
    if (std::holds_alternative<std::shared_ptr<Dict>>(obj)) {
        for (const auto & [_, a] : std::get<std::shared_ptr<Dict>>(obj)->value) {
            if (!holds_reduced(a)) {
                return false;
            }
        }
        return true;
    }
    return false;
}

bool holds_reduced(const Object & obj) {
    return (std::holds_alternative<std::shared_ptr<String>>(obj) ||
            std::holds_alternative<std::shared_ptr<Boolean>>(obj) ||
            std::holds_alternative<std::shared_ptr<Number>>(obj) ||
            std::holds_alternative<std::shared_ptr<File>>(obj) ||
            std::holds_alternative<std::shared_ptr<Executable>>(obj) ||
            std::holds_alternative<std::shared_ptr<StaticLibrary>>(obj) ||
            std::holds_alternative<std::shared_ptr<IncludeDirectories>>(obj) ||
            std::holds_alternative<std::shared_ptr<Program>>(obj) ||
            std::holds_alternative<std::unique_ptr<Message>>(obj) || holds_reduced_array(obj) ||
            holds_reduced_dict(obj));
}

} // namespace

bool all_args_reduced(const std::vector<Object> & pos_args,
                      const std::unordered_map<std::string, Object> & kw_args) {
    for (const auto & p : pos_args) {
        if (!holds_reduced(p)) {
            return false;
        }
    }
    for (const auto & [_, p] : kw_args) {
        if (!holds_reduced(p)) {
            return false;
        }
    }
    return true;
}

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
    // TODO: I don't want this in here, I'd rather have this all done in the backend, I think
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
        || function_walker(block, lower_messages)
        || function_walker(block, lower_assert)
        || function_walker(block, [&](const Object & obj) { return lower_files(obj, pstate); })
        || function_walker(block, [&](const Object & obj) { return lower_include_dirs(obj, pstate); })
        || function_walker(block, [&](const Object & obj) { return lower_executable(obj, pstate); })
        || function_walker(block, [&](const Object & obj) { return lower_static_library(obj, pstate); })
        ;
    // clang-format on
}

} // namespace MIR::Passes
