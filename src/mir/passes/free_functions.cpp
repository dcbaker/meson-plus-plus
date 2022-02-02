// SPDX-license-identifier: Apache-2.0
// Copyright © 2021-2022 Dylan Baker

#include <iostream>
#include <vector>

#include "argument_extractors.hpp"
#include "exceptions.hpp"
#include "log.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

// XXX: we probably need access to the source_root and build_root
std::optional<Object> lower_files(const FunctionCall & f, const State::Persistant & pstate) {
    std::vector<Object> files{};
    for (const auto & arg_h : f.pos_args) {
        // XXX: do something more realistic here
        // This could be Array<STring> and still be valid.
        if (!std::holds_alternative<std::shared_ptr<String>>(arg_h)) {
            throw Util::Exceptions::InvalidArguments("Arguments to 'files()' must be strings");
        }
        auto const & v = *std::get<std::shared_ptr<String>>(arg_h);

        files.emplace_back(std::make_shared<File>(v.value, f.source_dir, false, pstate.source_root,
                                                  pstate.build_root));
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
Source src_to_file(const Object & raw_src, const State::Persistant & pstate,
                   const std::string & subdir) {
    if (std::holds_alternative<std::shared_ptr<String>>(raw_src)) {
        const auto & src = *std::get<std::shared_ptr<String>>(raw_src);
        return std::make_shared<File>(src.value, subdir, false, pstate.source_root,
                                      pstate.build_root);
    } else if (std::holds_alternative<std::shared_ptr<File>>(raw_src)) {
        return std::get<std::shared_ptr<File>>(raw_src);
    } else if (std::holds_alternative<std::shared_ptr<CustomTarget>>(raw_src)) {
        return std::get<std::shared_ptr<CustomTarget>>(raw_src);
    } else {
        // TODO: there are other valid types here, like generator output and custom targets
        throw Util::Exceptions::InvalidArguments{
            "'executable' sources must be strings, files, or custom_target objects."};
    }
}

inline std::unordered_map<Toolchain::Language, std::vector<Arguments::Argument>>
target_arguments(const FunctionCall & f, const State::Persistant & pstate) {
    std::unordered_map<Toolchain::Language, std::vector<Arguments::Argument>> args{};
    const auto & comp_at = pstate.toolchains.find(Toolchain::Language::CPP);
    if (comp_at == pstate.toolchains.end()) {
        // TODO: better error message
        throw Util::Exceptions::MesonException(
            "Tried to build a C++ target without a C++ toolchain.");
    }

    const auto & comp = comp_at->second.build()->compiler;
    auto raw_args = extract_keyword_argument_a<std::shared_ptr<String>>(f.kw_args, "cpp_args");
    for (const auto & ra : raw_args) {
        args[Toolchain::Language::CPP].emplace_back(comp->generalize_argument(ra->value));
    }

    return args;
}

inline std::vector<StaticLinkage>
target_kwargs(const std::unordered_map<std::string, Object> & kwargs) {
    std::vector<StaticLinkage> s_link{};
    auto raw_args = extract_keyword_argument_a<std::shared_ptr<StaticLibrary>>(kwargs, "link_with");
    for (const auto & s : raw_args) {
        s_link.emplace_back(StaticLinkage{StaticLinkMode::NORMAL, s.get()});
    }

    return s_link;
}

template <typename T>
std::optional<std::shared_ptr<T>> lower_build_target(const FunctionCall & f,
                                                     const State::Persistant & pstate) {
    std::string f_name;
    if constexpr (std::is_same<T, Executable>::value) {
        f_name = "executable";
    } else if constexpr (std::is_same<T, StaticLibrary>::value) {
        f_name = "static_library";
    } else {
        assert(false);
    }

    // This doesn't handle the listified version corretly
    if (f.pos_args.size() < 2) {
        throw Util::Exceptions::InvalidArguments{f.name + " requires at least 2 arguments"};
    }

    auto pos_itr = f.pos_args.begin();

    const auto & name = extract_positional_argument<std::shared_ptr<String>>(*pos_itr++);
    if (!name) {
        throw Util::Exceptions::InvalidArguments{f.name + " first argument must be a string"};
    }

    // skip the first argument
    std::vector<Source> srcs{};
    for (; pos_itr != f.pos_args.end(); ++pos_itr) {
        srcs.emplace_back(src_to_file(*pos_itr, pstate, f.source_dir));
    }
    auto args = target_arguments(f, pstate);
    auto slink = target_kwargs(f.kw_args);
    auto raw_inc = extract_keyword_argument_a<std::shared_ptr<IncludeDirectories>>(
        f.kw_args, "include_directories", true);
    for (const auto & i : raw_inc) {
        for (const auto & d : i->directories) {
            args[Toolchain::Language::CPP].emplace_back(Arguments::Argument{
                d, Arguments::Type::INCLUDE,
                i->is_system ? Arguments::IncludeType::SYSTEM : Arguments::IncludeType::BASE});
        }
    }

    // TODO: machine parameter needs to be set from the native kwarg
    return std::make_shared<T>(name.value()->value, srcs, Machines::Machine::BUILD, f.source_dir,
                               args, slink, f.var);
}

std::optional<Object> lower_include_dirs(const FunctionCall & f, const State::Persistant & pstate) {
    for (const auto & a : f.pos_args) {
        if (!std::holds_alternative<std::shared_ptr<String>>(a)) {
            throw Util::Exceptions::InvalidArguments{
                "include_directories: all positional arguments must be strings"};
        }
    }

    std::vector<std::string> dirs{};
    for (const auto & a : f.pos_args) {
        dirs.emplace_back(std::get<std::shared_ptr<String>>(a)->value);
    }

    auto is_system = extract_keyword_argument<std::shared_ptr<Boolean>>(f.kw_args, "is_system")
                         .value_or(std::make_shared<Boolean>(false));

    return std::make_shared<IncludeDirectories>(dirs, is_system->value, f.var);
}

std::optional<Object> lower_messages(const FunctionCall & f) {
    MessageLevel level;
    if (f.name == "message") {
        level = MessageLevel::MESSAGE;
    } else if (f.name == "warning") {
        level = MessageLevel::WARN;
    } else if (f.name == "error") {
        level = MessageLevel::ERROR;
    }

    // TODO: Meson accepts anything as a message bascially, without flattening.
    // Currently, Meson++ flattens everything so I'm only going to allow strings for the moment.
    auto args =
        extract_variadic_arguments<std::shared_ptr<String>>(f.pos_args.begin(), f.pos_args.end());

    std::string message{};
    for (const auto & a : args) {
        if (!message.empty()) {
            message.append(" ");
        }
        message.append(a->value);
    }

    return std::make_unique<Message>(level, message);
}

std::optional<Object> lower_assert(const FunctionCall & f) {
    if (f.pos_args.size() < 1 || f.pos_args.size() > 2) {
        throw Util::Exceptions::InvalidArguments("assert: takes 1 or 2 arguments, got " +
                                                 std::to_string(f.pos_args.size()));
    }

    const auto & value = extract_positional_argument<std::shared_ptr<Boolean>>(f.pos_args[0]);

    if (!value.value()->value) {
        // TODO: maye have an assert level of message?
        // TODO, how to get the original values of this?
        std::string message = "";
        if (f.pos_args.size() == 2) {
            message =
                extract_positional_argument<std::shared_ptr<String>>(f.pos_args[1]).value()->value;
        }
        return std::make_unique<Message>(MessageLevel::ERROR, "Assertion failed: " + message);
    }

    // TODO: it would be better to
    return std::make_unique<Empty>();
}

std::optional<Object> lower_not(const FunctionCall & f) {
    // TODO: is this code actually reachable?
    if (f.pos_args.size() != 1) {
        throw Util::Exceptions::InvalidArguments("not: takes 1 argument, got " +
                                                 std::to_string(f.pos_args.size()));
    }

    const auto & value = extract_positional_argument<std::shared_ptr<Boolean>>(f.pos_args[0]);

    return std::make_shared<Boolean>(!value.value()->value);
}

std::optional<Object> lower_neg(const FunctionCall & f) {
    // TODO: is this code actually reachable?
    if (f.pos_args.size() != 1) {
        throw Util::Exceptions::InvalidArguments("neg: takes 1 argument, got " +
                                                 std::to_string(f.pos_args.size()));
    }

    const auto & value = extract_positional_argument<std::shared_ptr<Number>>(f.pos_args[0]);

    return std::make_shared<Number>(-value.value()->value);
}

std::optional<Object> lower_eq(const FunctionCall & f) {
    // TODO: is this code actually reachable?
    if (f.pos_args.size() != 2) {
        throw Util::Exceptions::InvalidArguments("==: takes 2 argument, got " +
                                                 std::to_string(f.pos_args.size()));
    }

    const auto & lhs = f.pos_args[0];
    const auto & rhs = f.pos_args[1];

    const bool can_compare =
        lhs.index() == rhs.index() || lhs.valueless_by_exception() || rhs.valueless_by_exception();
    if (!can_compare) {
        // TODO: metter error message here
        throw Util::Exceptions::InvalidArguments("Trying to compare unlike types");
    }

    // Can't rely on variants equality, because we have either shared_ptr or
    // unique_ptr inside the variant. Since we know they hold the same type, we
    // can do this safely
    bool value;
    if (std::holds_alternative<std::shared_ptr<String>>(lhs)) {
        value = std::get<std::shared_ptr<String>>(lhs)->value ==
                std::get<std::shared_ptr<String>>(rhs)->value;
    } else if (std::holds_alternative<std::shared_ptr<Number>>(lhs)) {
        value = std::get<std::shared_ptr<Number>>(lhs)->value ==
                std::get<std::shared_ptr<Number>>(rhs)->value;
    } else if (std::holds_alternative<std::shared_ptr<Boolean>>(lhs)) {
        value = std::get<std::shared_ptr<Boolean>>(lhs)->value ==
                std::get<std::shared_ptr<Boolean>>(rhs)->value;
    } else {
        // TODO: better error message here
        throw Util::Exceptions::MesonException("This might be a bug, cannot compare types");
    }

    return std::make_shared<Boolean>(value, f.var);
}

std::optional<Object> lower_ne(const FunctionCall & f) {
    // TODO: is this code actually reachable?
    if (f.pos_args.size() != 2) {
        throw Util::Exceptions::InvalidArguments("!=: takes 2 argument, got " +
                                                 std::to_string(f.pos_args.size()));
    }

    const auto & lhs = f.pos_args[0];
    const auto & rhs = f.pos_args[1];

    const bool can_compare =
        lhs.index() == rhs.index() || lhs.valueless_by_exception() || rhs.valueless_by_exception();
    if (!can_compare) {
        // TODO: metter error message here
        throw Util::Exceptions::InvalidArguments("Trying to compare unlike types");
    }

    // Can't rely on variants equality, because we have either shared_ptr or
    // unique_ptr inside the variant. Since we know they hold the same type, we
    // can do this safely
    bool value;
    if (std::holds_alternative<std::shared_ptr<String>>(lhs)) {
        value = std::get<std::shared_ptr<String>>(lhs)->value !=
                std::get<std::shared_ptr<String>>(rhs)->value;
    } else if (std::holds_alternative<std::shared_ptr<Number>>(lhs)) {
        value = std::get<std::shared_ptr<Number>>(lhs)->value !=
                std::get<std::shared_ptr<Number>>(rhs)->value;
    } else if (std::holds_alternative<std::shared_ptr<Boolean>>(lhs)) {
        value = std::get<std::shared_ptr<Boolean>>(lhs)->value !=
                std::get<std::shared_ptr<Boolean>>(rhs)->value;
    } else {
        // TODO: better error message here
        throw Util::Exceptions::MesonException("This might be a bug, cannot compare types");
    }

    return std::make_shared<Boolean>(value, f.var);
}

std::optional<Object> lower_declare_dependency(const FunctionCall & f,
                                               const State::Persistant & pstate) {
    if (!f.pos_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "declare_dependency: takes 0 positional arguments.");
    }

    std::string version = extract_keyword_argument<std::shared_ptr<String>>(f.kw_args, "version")
                              .value_or(std::make_shared<String>("unknown"))
                              ->value;

    std::vector<Arguments::Argument> args{};

    const auto & raw_comp_args =
        extract_keyword_argument_a<std::shared_ptr<String>>(f.kw_args, "compile_args");
    if (!raw_comp_args.empty()) {
        // XXX: this assumes C++
        // should this always use gcc/g++?
        const auto & comp_at = pstate.toolchains.find(Toolchain::Language::CPP);
        if (comp_at == pstate.toolchains.end()) {
            // TODO: better error message
            throw Util::Exceptions::MesonException(
                "Tried to build a C++ target without a C++ toolchain.");
        }
        const auto & comp = comp_at->second.build()->compiler;

        for (const auto & ra : raw_comp_args) {
            args.emplace_back(comp->generalize_argument(ra->value));
        }
    }

    const auto & raw_inc_args =
        extract_keyword_argument_av<std::shared_ptr<String>, std::shared_ptr<IncludeDirectories>>(
            f.kw_args, "include_directories");

    for (const auto & i : raw_inc_args) {
        if (std::holds_alternative<std::shared_ptr<String>>(i)) {
            const auto & s = *std::get<std::shared_ptr<String>>(i);
            args.emplace_back(Arguments::Argument{s.value, Arguments::Type::INCLUDE,
                                                  Arguments::IncludeType::BASE});
        } else {
            const auto & inc = *std::get<std::shared_ptr<IncludeDirectories>>(i);
            for (const auto & d : inc.directories) {
                args.emplace_back(Arguments::Argument{
                    d, Arguments::Type::INCLUDE,
                    inc.is_system ? Arguments::IncludeType::SYSTEM : Arguments::IncludeType::BASE});
            }
        }
    }

    const auto & raw_deps =
        extract_keyword_argument_a<std::shared_ptr<Dependency>>(f.kw_args, "dependencies");
    for (const auto & d : raw_deps) {
        for (const auto & a : d->arguments) {
            args.emplace_back(a);
        }
    }

    return std::make_shared<Dependency>("internal", true, version, args, f.var);
}

Source extract_source(const Object & obj, const fs::path & current_source_dir,
                      const State::Persistant & pstate) {
    if (std::holds_alternative<std::shared_ptr<CustomTarget>>(obj)) {
        return std::get<std::shared_ptr<CustomTarget>>(obj);
    } else if (std::holds_alternative<std::shared_ptr<File>>(obj)) {
        return std::get<std::shared_ptr<File>>(obj);
    } else if (std::holds_alternative<std::shared_ptr<String>>(obj)) {
        const auto & str = *std::get<std::shared_ptr<String>>(obj);
        return std::make_shared<File>(str.value, current_source_dir, false, pstate.source_root,
                                      pstate.build_root);
    } else {
        // TODO: better error
        throw Util::Exceptions::InvalidArguments("custom_target: 'input' keyword argument must "
                                                 "be 'custom_target', 'string', or 'file'");
    }
}

std::vector<Source> extract_source_inputs(const std::unordered_map<std::string, Object> & kws,
                                          const std::string & name,
                                          const fs::path & current_source_dir,
                                          const State::Persistant & pstate) {
    const auto & obj = kws.find(name);
    if (obj == kws.end()) {
        return {};
    }

    std::vector<Source> srcs{};
    if (std::holds_alternative<std::shared_ptr<Array>>(obj->second)) {
        for (const auto & o : std::get<std::shared_ptr<Array>>(obj->second)->value) {
            srcs.emplace_back(extract_source(o, current_source_dir, pstate));
        }
    } else {
        srcs.emplace_back(extract_source(obj->second, current_source_dir, pstate));
    }

    return srcs;
}

std::vector<std::string> extract_ct_command(const Object & obj, const std::vector<Source> & inputs,
                                            const std::vector<File> & outputs) {

    if (std::holds_alternative<std::shared_ptr<String>>(obj)) {
        // TODO: in c++20 these can be constexpr
        static const auto output_size = std::string{"@OUTPUT"}.size();
        static const auto input_size = std::string{"@INPUT"}.size();

        const auto & v = std::get<std::shared_ptr<String>>(obj)->value;
        // TODO: indexed input and output arguments
        if (v == "@OUTPUT@") {
            std::vector<std::string> outs{};
            for (const auto & o : outputs) {
                outs.emplace_back(o.relative_to_build_dir());
            }
            return outs;
        } else if (v.substr(0, output_size) == "@OUTPUT") {
            const auto & index = std::stoul(v.substr(output_size, v.size() - 1));
            return {outputs[index].relative_to_build_dir()};
        } else if (v == "@INPUT@") {
            std::vector<std::string> ins{};
            for (const auto & o : inputs) {
                if (std::holds_alternative<std::shared_ptr<File>>(o)) {
                    ins.emplace_back(std::get<std::shared_ptr<File>>(o)->relative_to_build_dir());
                } else {
                    const auto & t = *std::get<std::shared_ptr<CustomTarget>>(o);
                    for (const auto & o2 : t.outputs) {
                        ins.emplace_back(o2.relative_to_build_dir());
                    }
                }
            }
            return ins;
        } else if (v.substr(0, input_size) == "@INPUT") {
            const auto & index = std::stoul(v.substr(input_size, v.size() - 1));
            const Source s = inputs[index];
            if (std::holds_alternative<std::shared_ptr<File>>(s)) {
                return {std::get<std::shared_ptr<File>>(s)->relative_to_build_dir()};
            } else {
                std::vector<std::string> outs{};
                const auto & t = *std::get<std::shared_ptr<CustomTarget>>(s);
                // TODO: get the right index
            }
        } else {
            return {v};
        }
    } else if (std::holds_alternative<std::shared_ptr<File>>(obj)) {
        return {std::get<std::shared_ptr<File>>(obj)->relative_to_build_dir()};
    } else if (std::holds_alternative<std::shared_ptr<Program>>(obj)) {
        return {std::get<std::shared_ptr<Program>>(obj)->path};
    }
    throw Util::Exceptions::InvalidArguments(
        "custom_target: 'commands' must be strings, files, or find_program objects");
}

std::vector<std::string> extract_ct_command(const std::unordered_map<std::string, Object> & kws,
                                            const std::vector<Source> & inputs,
                                            const std::vector<File> & outputs) {
    const auto & cmd_obj = kws.find("command");
    if (cmd_obj == kws.end()) {
        throw Util::Exceptions::MesonException("custom_target: missing required kwarg 'command'");
    }

    std::vector<std::string> command{};
    if (std::holds_alternative<std::shared_ptr<Array>>(cmd_obj->second)) {
        for (const auto & o : std::get<std::shared_ptr<Array>>(cmd_obj->second)->value) {
            const auto & e = extract_ct_command(o, inputs, outputs);
            command.insert(command.end(), e.begin(), e.end());
        }
    } else {
        const auto & e = extract_ct_command(cmd_obj->second, inputs, outputs);
        command.insert(command.end(), e.begin(), e.end());
    }

    return command;
}

std::optional<Object> lower_custom_target(const FunctionCall & func,
                                          const State::Persistant & pstate) {
    const auto & inputs = extract_source_inputs(func.kw_args, "input", func.source_dir, pstate);

    std::vector<File> outputs{};
    for (const auto & a :
         extract_keyword_argument_a<std::shared_ptr<String>>(func.kw_args, "output")) {
        outputs.emplace_back(
            File{a->value, func.source_dir, true, pstate.source_root, pstate.build_root});
    }

    const auto & raw_name = extract_positional_argument<std::shared_ptr<String>>(func.pos_args[0]);
    const std::string & name = raw_name ? raw_name.value()->value : outputs[0].name;

    // TODO: output and input substitution
    const auto & command = extract_ct_command(func.kw_args, inputs, outputs);

    return std::make_shared<CustomTarget>(name, inputs, outputs, command, func.source_dir,
                                          func.var);
}

bool holds_reduced(const Object & obj);

bool holds_reduced_array(const Object & obj) {
    if (std::holds_alternative<std::shared_ptr<Array>>(obj)) {
        for (const auto & a : std::get<std::shared_ptr<Array>>(obj)->value) {
            if (!holds_reduced(a)) {
                return false;
            } else if (std::holds_alternative<std::shared_ptr<Array>>(a)) {
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
            std::holds_alternative<std::shared_ptr<CustomTarget>>(obj) ||
            std::holds_alternative<std::shared_ptr<Dependency>>(obj) ||
            std::holds_alternative<std::unique_ptr<Message>>(obj) || holds_reduced_array(obj) ||
            holds_reduced_dict(obj));
}

std::optional<Object> lower_free_funcs_impl(const Object & obj, const State::Persistant & pstate) {
    if (!std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        return std::nullopt;
    }
    const auto & f = *std::get<std::shared_ptr<FunctionCall>>(obj);

    // This is not a free function
    if (f.holder.has_value()) {
        return std::nullopt;
    }

    if (!all_args_reduced(f.pos_args, f.kw_args)) {
        return std::nullopt;
    }

    if (f.name == "rel_eq") {
        return lower_eq(f);
    } else if (f.name == "rel_ne") {
        return lower_ne(f);
    } else if (f.name == "unary_not") {
        return lower_not(f);
    } else if (f.name == "unary_neg") {
        return lower_neg(f);
    } else if (f.name == "assert") {
        return lower_assert(f);
    } else if (f.name == "message" || f.name == "warning" || f.name == "error") {
        return lower_messages(f);
    } else if (f.name == "include_directories") {
        return lower_include_dirs(f, pstate);
    } else if (f.name == "files") {
        return lower_files(f, pstate);
    } else if (f.name == "custom_target") {
        return lower_custom_target(f, pstate);
    } else if (f.name == "executable") {
        return lower_build_target<Executable>(f, pstate);
    } else if (f.name == "static_library") {
        return lower_build_target<StaticLibrary>(f, pstate);
    } else if (f.name == "declare_dependency") {
        return lower_declare_dependency(f, pstate);
    }

    // XXX: Shouldn't really be able to get here...
    return std::nullopt;
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
    const auto & f = *std::get<std::shared_ptr<FunctionCall>>(obj);

    if (f.name != "project") {
        throw Util::Exceptions::MesonException{
            "First non-whitespace, non-comment must be a call to project()"};
    }

    // This doesn't handle the listified version corretly
    if (f.pos_args.size() < 1) {
        throw Util::Exceptions::InvalidArguments{"project requires at least 1 argument"};
    }

    auto pos = f.pos_args.begin();

    pstate.name = std::get<std::shared_ptr<String>>(*pos++)->value;
    // TODO: I don't want this in here, I'd rather have this all done in the backend, I think
    std::cout << "Project name: " << Util::Log::bold(pstate.name) << std::endl;

    const auto & langs = extract_variadic_arguments<std::shared_ptr<String>>(pos, f.pos_args.end());
    for (const auto & lang : langs) {
        const auto l = Toolchain::from_string(lang->value);

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
    return function_walker(block,
                           [&](const Object & obj) { return lower_free_funcs_impl(obj, pstate); });
}

} // namespace MIR::Passes
