// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include "argument_extractors.hpp"
#include "argument_validators.hpp"
#include "exceptions.hpp"
#include "log.hpp"
#include "passes.hpp"
#include "private.hpp"

#include <algorithm>
#include <iostream>
#include <vector>

namespace MIR::Passes {

namespace {

std::optional<Instruction> lower_files(const FunctionCall & func,
                                       const State::Persistant & pstate) {
    ArgumentValidator::Files args = ArgumentValidator::parse_files(func, pstate);
    std::vector<Instruction> files{};
    files.reserve(args.files.size());
    for (auto && f : args.files) {
        files.emplace_back(std::move(f));
    }
    return Array{std::move(files)};
}

/**
 * Convert source to file
 *
 * This is only for Files, not for targets. I want to separate targets into a
 * separate structure (or multiple structures, probably).
 *
 * This walks a vector of Instructions, creating a new, flat vector of Files,
 * converting any strings into files, appending files as is, and flattening any
 * arrays it runs into.
 */
Instruction src_to_file(const Instruction & raw_src, const State::Persistant & pstate,
                        const std::string & subdir) {
    if (std::holds_alternative<String>(*raw_src.obj_ptr)) {
        const auto & src = std::get<String>(*raw_src.obj_ptr);
        return File{src.value, subdir, false, pstate.source_root, pstate.build_root};
    }
    if (std::holds_alternative<File>(*raw_src.obj_ptr) ||
        std::holds_alternative<CustomTarget>(*raw_src.obj_ptr)) {
        return raw_src;
    }
    throw Util::Exceptions::InvalidArguments{
        "'executable' sources must be strings, files, or custom_target objects."};
}

template <typename T>
std::optional<T> lower_build_target(const FunctionCall & f, const State::Persistant & pstate) {
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

    const auto & name = extract_positional_argument<String>(
        *pos_itr->obj_ptr, f.name + " first argument must be a string");
    ++pos_itr;

    // skip the first argument
    std::vector<Instruction> srcs{};
    for (; pos_itr != f.pos_args.end(); ++pos_itr) {
        srcs.emplace_back(src_to_file(*pos_itr, pstate, f.source_dir));
    }

    std::unordered_map<Toolchain::Language, std::vector<Arguments::Argument>> args{};
    const auto & comp_at = pstate.toolchains.find(Toolchain::Language::CPP);
    if (comp_at == pstate.toolchains.end()) {
        // TODO: better error message
        throw Util::Exceptions::MesonException(
            "Tried to build a C++ target without a C++ toolchain.");
    }

    const auto & comp = comp_at->second.build()->compiler;
    auto raw_args = extract_keyword_argument_a<String>(f.kw_args, "cpp_args",
                                                       f.name + ": cpp_arguments must be strings")
                        .value_or(std::vector<String>{});
    for (const auto & ra : raw_args) {
        args[Toolchain::Language::CPP].emplace_back(comp->generalize_argument(ra.value));
    }

    // XXX: is this going to work, or are we going to end up taking a pointer to a temporary?
    // TODO: validation
    std::vector<StaticLinkage> slink{};
    auto raw_link_with =
        extract_keyword_argument_a<StaticLibrary>(
            f.kw_args, "link_with",
            f.name + ": 'link_with' keyword argument must be StaticLibrary objects")
            .value_or(std::vector<StaticLibrary>{});
    slink.reserve(raw_link_with.size());
    for (const auto & s : raw_link_with) {
        slink.emplace_back(StaticLinkMode::NORMAL, s);
    }

    auto raw_inc =
        extract_keyword_argument_a<IncludeDirectories>(
            f.kw_args, "include_directories",
            f.name + ": include_directories keyword argument must be IncludeDirectory objects")
            .value_or(std::vector<IncludeDirectories>{});
    for (const auto & i : raw_inc) {
        for (const auto & d : i.directories) {
            args[Toolchain::Language::CPP].emplace_back(d, Arguments::Type::INCLUDE,
                                                        i.is_system ? Arguments::IncludeType::SYSTEM
                                                                    : Arguments::IncludeType::BASE);
        }
    }

    const auto & deps = extract_keyword_argument_a<Dependency>(
                            f.kw_args, "dependencies",
                            f.name + ": dependencies keyword argument must be Dependency objects")
                            .value_or(std::vector<Dependency>{});
    for (const auto & d : deps) {
        for (const auto & a : d.arguments) {
            args[Toolchain::Language::CPP].emplace_back(a);
        }
    }

    // TODO: machine parameter needs to be set from the native kwarg
    return T{name.value, srcs, Machines::Machine::BUILD, f.source_dir, args, slink};
}

std::optional<Instruction> lower_include_dirs(const FunctionCall & f,
                                              const State::Persistant & pstate) {
    for (const auto & a : f.pos_args) {
        if (!std::holds_alternative<String>(*a.obj_ptr)) {
            throw Util::Exceptions::InvalidArguments{
                "include_directories: all positional arguments must be strings"};
        }
    }

    std::vector<std::string> dirs{};
    for (const auto & a : f.pos_args) {
        dirs.emplace_back(std::get<String>(*a.obj_ptr).value);
    }

    auto is_system =
        extract_keyword_argument<Boolean>(
            f.kw_args, "is_system", "include_directories: 'is_system' argument must be a boolean")
            .value_or(Boolean{false});

    return IncludeDirectories{dirs, is_system.value};
}

std::optional<Instruction> lower_messages(const FunctionCall & f) {
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
    auto args = extract_variadic_arguments<String>(f.pos_args.begin(), f.pos_args.end(),
                                                   "message: arguments must be strings");

    std::string message{};
    for (const auto & a : args) {
        if (!message.empty()) {
            message.append(" ");
        }
        message.append(a.value);
    }

    return Message{level, message};
}

std::optional<Instruction> lower_assert(const FunctionCall & f) {
    if (f.pos_args.empty() || f.pos_args.size() > 2) {
        throw Util::Exceptions::InvalidArguments("assert: takes 1 or 2 arguments, got " +
                                                 std::to_string(f.pos_args.size()));
    }

    const auto & value = extract_positional_argument<Boolean>(
        f.pos_args[0], f.name + ": First argument did not resolve to boolean");

    if (!value.value) {
        // TODO: maye have an assert level of message?
        // TODO, how to get the original values of this?
        std::string message;
        if (f.pos_args.size() == 2) {
            message = extract_positional_argument<String>(f.pos_args[1]).value().value;
        }
        return Message{MessageLevel::ERROR, "Assertion failed: " + message};
    }

    // TODO: it would be better to
    return Instruction{std::monostate{}};
}

std::optional<Instruction> lower_not(const FunctionCall & f) {
    // TODO: is this code actually reachable?
    if (f.pos_args.size() != 1) {
        throw Util::Exceptions::InvalidArguments("not: takes 1 argument, got " +
                                                 std::to_string(f.pos_args.size()));
    }

    const auto & value = extract_positional_argument<Boolean>(
        f.pos_args[0], f.name + ": attempted to negate a value that did not resolve to a boolean");

    return Boolean{!value.value};
}

std::optional<Instruction> lower_neg(const FunctionCall & f) {
    // TODO: is this code actually reachable?
    if (f.pos_args.size() != 1) {
        throw Util::Exceptions::InvalidArguments("neg: takes 1 argument, got " +
                                                 std::to_string(f.pos_args.size()));
    }

    const auto & value = extract_positional_argument<Number>(
        f.pos_args[0], f.name + ": attempted to negate a value that did not resolve to a number");

    return Number{-value.value};
}

std::optional<Instruction> lower_eq(const FunctionCall & f) {
    // TODO: is this code actually reachable?
    if (f.pos_args.size() != 2) {
        throw Util::Exceptions::InvalidArguments("==: takes 2 argument, got " +
                                                 std::to_string(f.pos_args.size()));
    }

    const auto & lhs = f.pos_args[0];
    const auto & rhs = f.pos_args[1];

    const bool can_compare = lhs.obj_ptr->index() == rhs.obj_ptr->index() ||
                             lhs.obj_ptr->valueless_by_exception() ||
                             rhs.obj_ptr->valueless_by_exception();
    if (!can_compare) {
        // TODO: metter error message here
        throw Util::Exceptions::InvalidArguments("Trying to compare unlike types");
    }

    // TODO: if all Object types were comparible then we could just lhs == rhs
    // and be done with it
    // We know that both types are the same already
    bool value;
    if (std::holds_alternative<String>(*lhs.obj_ptr)) {
        value = std::get<String>(*lhs.obj_ptr) == std::get<String>(*rhs.obj_ptr);
    } else if (std::holds_alternative<Number>(*lhs.obj_ptr)) {
        value = std::get<Number>(*lhs.obj_ptr) == std::get<Number>(*rhs.obj_ptr);
    } else if (std::holds_alternative<Boolean>(*lhs.obj_ptr)) {
        value = std::get<Boolean>(*lhs.obj_ptr) == std::get<Boolean>(*rhs.obj_ptr);
    } else {
        throw Util::Exceptions::MesonException{"TODO: missing comparison operator"};
    }

    return Boolean{value};
}

std::optional<Instruction> lower_ne(const FunctionCall & f) {
    // TODO: is this code actually reachable?
    if (f.pos_args.size() != 2) {
        throw Util::Exceptions::InvalidArguments("!=: takes 2 argument, got " +
                                                 std::to_string(f.pos_args.size()));
    }

    const auto & lhs = f.pos_args[0];
    const auto & rhs = f.pos_args[1];

    const bool can_compare = lhs.obj_ptr->index() == rhs.obj_ptr->index() ||
                             lhs.obj_ptr->valueless_by_exception() ||
                             rhs.obj_ptr->valueless_by_exception();
    if (!can_compare) {
        // TODO: metter error message here
        throw Util::Exceptions::InvalidArguments("Trying to compare unlike types");
    }

    // We know that both types are the same already
    bool value;
    if (std::holds_alternative<String>(*lhs.obj_ptr)) {
        value = std::get<String>(*lhs.obj_ptr) != std::get<String>(*rhs.obj_ptr);
    } else if (std::holds_alternative<Number>(*lhs.obj_ptr)) {
        value = std::get<Number>(*lhs.obj_ptr) != std::get<Number>(*rhs.obj_ptr);
    } else if (std::holds_alternative<Boolean>(*lhs.obj_ptr)) {
        value = std::get<Boolean>(*lhs.obj_ptr) != std::get<Boolean>(*rhs.obj_ptr);
    } else {
        throw Util::Exceptions::MesonException{"TODO: missing comparison operator"};
    }

    return Boolean{value};
}

std::optional<Instruction> lower_declare_dependency(const FunctionCall & f,
                                                    const State::Persistant & pstate) {
    if (!f.pos_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "declare_dependency: takes 0 positional arguments.");
    }

    std::string version =
        extract_keyword_argument<String>(
            f.kw_args, "version", "declare_dependency: 'version' keyword argument must be a string")
            .value_or(String("unknown"))
            .value;

    std::vector<Arguments::Argument> args{};
    const auto & raw_comp_args = extract_keyword_argument_a<String>(
        f.kw_args, "compile_args", f.name + ": 'compile_args' keyword argument must be strings");
    if (raw_comp_args) {
        // XXX: this assumes C++
        // should this always use gcc/g++?
        const auto & comp_at = pstate.toolchains.find(Toolchain::Language::CPP);
        if (comp_at == pstate.toolchains.end()) {
            // TODO: better error message
            throw Util::Exceptions::MesonException(
                "Tried to build a C++ target without a C++ toolchain.");
        }
        const auto & comp = comp_at->second.build()->compiler;

        for (const auto & ra : raw_comp_args.value()) {
            args.emplace_back(comp->generalize_argument(ra.value));
        }
    }

    const auto & raw_inc_args =
        extract_keyword_argument_av<String, IncludeDirectories>(
            f.kw_args, "include_directories",
            f.name + ": 'include_directories' must be strings or IncludeDirectories objects")
            .value_or(std::vector<std::variant<String, IncludeDirectories>>{});

    for (const auto & i : raw_inc_args) {
        if (std::holds_alternative<String>(i)) {
            const auto & s = std::get<String>(i);
            args.emplace_back(s.value, Arguments::Type::INCLUDE, Arguments::IncludeType::BASE);
        } else {
            const auto & inc = std::get<IncludeDirectories>(i);
            for (const auto & d : inc.directories) {
                args.emplace_back(d, Arguments::Type::INCLUDE,
                                  inc.is_system ? Arguments::IncludeType::SYSTEM
                                                : Arguments::IncludeType::BASE);
            }
        }
    }

    const auto & raw_deps =
        extract_keyword_argument_a<Dependency>(
            f.kw_args, "dependencies",
            f.name + ": 'dependencies' keyword argument must be Dependency objects")
            .value_or(std::vector<Dependency>{});
    for (const auto & d : raw_deps) {
        auto dargs = d.arguments;
        std::copy(dargs.begin(), dargs.end(), std::back_inserter(args));
    }

    return Dependency{"internal", true, version, args};
}

class CallableReducer {
  public:
    CallableReducer() = default;

    Callable operator()(const std::monostate &) {
        throw Util::Exceptions::InvalidArguments(
            "test: second argument must be a File, Executable, or Found Program");
    }
    Callable operator()(const MIR::File & f) { return f; }
    Callable operator()(const MIR::Executable & f) { return f; }
    Callable operator()(const MIR::Program & f) { return f; }
};

struct OutputReducer {
    std::vector<std::variant<MIR::String, MIR::File>> operator()(MIR::String s) { return {s}; }
    std::vector<std::variant<MIR::String, MIR::File>> operator()(MIR::File f) { return {f}; }
    std::vector<std::variant<MIR::String, MIR::File>> operator()(const MIR::CustomTarget & t) {
        std::vector<std::variant<MIR::String, MIR::File>> out;
        for (auto && o : t.outputs) {
            out.push_back(o);
        }
        return out;
    }
};

std::optional<Instruction> lower_test(const FunctionCall & f, const State::Persistant & pstate) {
    if (f.pos_args.size() != 2) {
        throw Util::Exceptions::InvalidArguments("test: takes 2 positional arguments.");
    }

    auto && name = extract_positional_argument<String>(
        f.pos_args.at(0), f.name + ": first argument must be a string");

    // TODO: should also allow CustomTarget and Jar
    auto && prog_v = extract_positional_argument_v<MIR::File, MIR::Program, MIR::Executable>(
        f.pos_args.at(1), f.name + ": got an invalid type for program");
    const Callable & prog = std::visit(CallableReducer{}, prog_v);

    // TODO: Also allows targets
    auto && raw_args =
        extract_keyword_argument_av<MIR::String, MIR::File, MIR::CustomTarget>(
            f.kw_args, "args",
            f.name + ": 'args' keyword arguments must be strings, files, or custom_target objects")
            .value_or(std::vector<std::variant<MIR::String, MIR::File, MIR::CustomTarget>>{});
    std::vector<std::variant<MIR::String, MIR::File>> arguments;
    for (auto && a : raw_args) {
        for (auto && n : std::visit(OutputReducer{}, a)) {
            arguments.push_back(n);
        }
    }

    const bool xfail =
        extract_keyword_argument<MIR::Boolean>(f.kw_args, "should_fail",
                                               "test: 'should_fail' argument must be a boolean")
            .value_or(MIR::Boolean{false})
            .value;

    return Test{name.value, prog, arguments, xfail};
}

std::vector<Instruction>
extract_source_inputs(const std::unordered_map<std::string, Instruction> & kws,
                      const std::string & name, const fs::path & current_source_dir,
                      const State::Persistant & pstate) {
    const auto & obj = kws.find(name);
    if (obj == kws.end()) {
        return {};
    }

    std::vector<Instruction> srcs{};
    if (std::holds_alternative<Array>(*obj->second.obj_ptr)) {
        for (const auto & o : std::get<Array>(*obj->second.obj_ptr).value) {
            srcs.emplace_back(src_to_file(o, pstate, current_source_dir));
        }
    } else {
        srcs.emplace_back(src_to_file(obj->second, pstate, current_source_dir));
    }

    return srcs;
}

std::vector<std::string> extract_ct_command(const Instruction & obj,
                                            const std::vector<Instruction> & inputs,
                                            const std::vector<File> & outputs) {

    if (std::holds_alternative<String>(*obj.obj_ptr)) {
        // TODO: in c++20 these can be constexpr
        static const auto output_size = std::string{"@OUTPUT"}.size();
        static const auto input_size = std::string{"@INPUT"}.size();

        const auto & v = std::get<String>(*obj.obj_ptr).value;
        // TODO: indexed input and output arguments
        if (v == "@OUTPUT@") {
            std::vector<std::string> outs{};
            outs.reserve(outputs.size());
            for (const auto & o : outputs) {
                outs.push_back(o.relative_to_build_dir());
            }
            return outs;
        }
        if (v.substr(0, output_size) == "@OUTPUT") {
            const auto & index = std::stoul(v.substr(output_size, v.size() - 1));
            return {outputs[index].relative_to_build_dir()};
        }
        if (v == "@INPUT@") {
            std::vector<std::string> ins{};
            for (const auto & o : inputs) {
                if (std::holds_alternative<File>(*o.obj_ptr)) {
                    ins.emplace_back(std::get<File>(*o.obj_ptr).relative_to_build_dir());
                } else {
                    const auto & t = std::get<CustomTarget>(*o.obj_ptr);
                    for (const auto & o2 : t.outputs) {
                        ins.emplace_back(o2.relative_to_build_dir());
                    }
                }
            }
            return ins;
        }
        if (v.substr(0, input_size) == "@INPUT") {
            const auto & index = std::stoul(v.substr(input_size, v.size() - 1));
            const Instruction & s = inputs[index];
            if (std::holds_alternative<File>(*s.obj_ptr)) {
                return {std::get<File>(*s.obj_ptr).relative_to_build_dir()};
            }
            assert(false); // FIXME: inputs
        }
        return {v};
    }
    if (std::holds_alternative<File>(*obj.obj_ptr)) {
        return {std::get<File>(*obj.obj_ptr).relative_to_build_dir()};
    }
    if (std::holds_alternative<Program>(*obj.obj_ptr)) {
        return {std::get<Program>(*obj.obj_ptr).path};
    }
    throw Util::Exceptions::InvalidArguments(
        "custom_target: 'commands' must be strings, files, or find_program objects");
}

std::vector<std::string>
extract_ct_command(const std::unordered_map<std::string, Instruction> & kws,
                   const std::vector<Instruction> & inputs, const std::vector<File> & outputs) {
    const auto & cmd_obj = kws.find("command");
    if (cmd_obj == kws.end()) {
        throw Util::Exceptions::MesonException("custom_target: missing required kwarg 'command'");
    }

    std::vector<std::string> command{};
    if (std::holds_alternative<Array>(*cmd_obj->second.obj_ptr)) {
        for (const auto & o : std::get<Array>(*cmd_obj->second.obj_ptr).value) {
            const auto & e = extract_ct_command(o, inputs, outputs);
            command.insert(command.end(), e.begin(), e.end());
        }
    } else {
        const auto & e = extract_ct_command(cmd_obj->second, inputs, outputs);
        command.insert(command.end(), e.begin(), e.end());
    }

    return command;
}

std::optional<Instruction> lower_custom_target(const FunctionCall & func,
                                               const State::Persistant & pstate) {
    const auto & inputs = extract_source_inputs(func.kw_args, "input", func.source_dir, pstate);

    std::vector<File> outputs{};
    auto && raw_outs =
        extract_keyword_argument_a<String>(func.kw_args, "output",
                                           "custom_target: output arguments must be strings")
            .value_or(std::vector<String>{});
    for (const auto & a : raw_outs) {
        outputs.emplace_back(a.value, func.source_dir, true, pstate.source_root, pstate.build_root);
    }

    const auto & raw_name = extract_positional_argument<String>(func.pos_args[0]);
    const std::string & name = raw_name ? raw_name.value().value : outputs[0].name;

    // TODO: output and input substitution
    const auto & command = extract_ct_command(func.kw_args, inputs, outputs);

    return CustomTarget{name, inputs, outputs, command, func.source_dir, {}, std::nullopt};
}

enum class ArgumentScope {
    project_comp,
    project_link,
    global_comp,
    global_link,
};

std::optional<Instruction> lower_add_arguments(const FunctionCall & func, const ArgumentScope scope,
                                               const State::Persistant & pstate) {

    const auto langs = extract_keyword_argument_a<MIR::String>(
        func.kw_args, "language", func.name + ": 'language' keyword argument must be strings");
    if (!langs) {
        throw Util::Exceptions::MesonException(func.name + ": missing required kwarg 'language'");
    }

    const std::vector<MIR::String> arguments = extract_variadic_arguments<MIR::String>(
        func.pos_args.begin(), func.pos_args.end(),
        func.name + ": positional arguments must be strings");
    // Meson allows this, so if we don't get any arguments, just return an empty to delete the node
    if (arguments.empty()) {
        return Instruction{std::monostate{}};
    }

    ArgMap mapping;
    for (auto && s : langs.value()) {
        const Toolchain::Language lang = Toolchain::from_string(s.value);
        if (const auto & tc = pstate.toolchains.find(lang); tc != pstate.toolchains.end()) {
            for (auto && arg : arguments) {
                mapping[lang].emplace_back(
                    tc->second.build()->compiler->generalize_argument(arg.value));
            }
        }
    }

    return AddArguments{std::move(mapping), func.name.substr(0, 10) == "add_global"};
};

std::optional<Instruction> lower_vcs_tag(const FunctionCall & f, const State::Persistant & p) {
    if (!f.pos_args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "vcs_tag: does not take any positional arguments.");
    }
    if (f.kw_args.find("input") == f.kw_args.end()) {
        throw Util::Exceptions::InvalidArguments(
            "vcs_tag: missing required keyword argument input.");
    }
    if (f.kw_args.find("output") == f.kw_args.end()) {
        throw Util::Exceptions::InvalidArguments(
            "vcs_tag: missing required keyword argument output.");
    }
    if (f.kw_args.find("command") != f.kw_args.end()) {
        throw Util::Exceptions::MesonException(
            "Not implemented: vcs_tag 'command' keyword argument");
    }

    Instruction input = src_to_file(f.kw_args.find("input")->second, p, f.source_dir);
    auto output = extract_keyword_argument<MIR::String>(f.kw_args, "output",
                                                        f.name + ": 'output' must be a string")
                      .value();
    // TODO: get version from project() call
    auto fallback = extract_keyword_argument<MIR::String>(f.kw_args, "fallback",
                                                          f.name + ": 'fallback' must be a string")
                        .value_or(MIR::String{p.project_version});
    auto replace_string =
        extract_keyword_argument<MIR::String>(f.kw_args, "replace_string",
                                              f.name + ": 'replace_string' must be a string")
            .value_or(MIR::String{"@VCS_TAG@"});

    File outfile{output.value, f.source_dir, true, p.source_root, p.build_root};
    const auto & src = std::get<File>(*src_to_file(input, p, f.source_dir).obj_ptr);

    const std::string depfile = outfile.relative_to_build_dir().string() + ".d";

    // TODO: we'd really like to put the depfile in private dir, but we can't
    // resolve the private dir at the MIR level.
    std::vector<std::string> command{
        p.mesonpp,
        "vcs_tag",
        src.relative_to_build_dir(),
        outfile.relative_to_build_dir(),
        fallback.value,
        replace_string.value,
        p.source_root,
        depfile,
    };

    return CustomTarget{
        outfile.name, {std::move(input)}, {std::move(outfile)}, command, f.source_dir, {}, depfile,
    };
}

bool holds_reduced(const Instruction & obj);

bool holds_reduced_array(const Instruction & obj) {
    auto val = std::get_if<Array>(obj.obj_ptr.get());
    if (val != nullptr) {
        return std::none_of(val->value.begin(), val->value.end(), [](auto && a) {
            return !holds_reduced(a) || std::holds_alternative<Array>(*a.obj_ptr);
        });
    }
    return false;
}

bool holds_reduced_dict(const Instruction & obj) {
    auto val = std::get_if<Dict>(obj.obj_ptr.get());
    if (val != nullptr) {
        return std::all_of(val->value.begin(), val->value.end(),
                           [](auto && v) { return holds_reduced(*v.second.obj_ptr); });
    }
    return false;
}

bool holds_reduced(const Instruction & obj) {
    return (std::holds_alternative<String>(*obj.obj_ptr) ||
            std::holds_alternative<Boolean>(*obj.obj_ptr) ||
            std::holds_alternative<Number>(*obj.obj_ptr) ||
            std::holds_alternative<File>(*obj.obj_ptr) ||
            std::holds_alternative<Executable>(*obj.obj_ptr) ||
            std::holds_alternative<StaticLibrary>(*obj.obj_ptr) ||
            std::holds_alternative<IncludeDirectories>(*obj.obj_ptr) ||
            std::holds_alternative<Program>(*obj.obj_ptr) ||
            std::holds_alternative<CustomTarget>(*obj.obj_ptr) ||
            std::holds_alternative<Dependency>(*obj.obj_ptr) ||
            std::holds_alternative<Message>(*obj.obj_ptr) || holds_reduced_array(obj) ||
            holds_reduced_dict(obj));
}

} // namespace

std::optional<Instruction> lower_free_functions(const Instruction & obj,
                                                const State::Persistant & pstate) {
    if (!std::holds_alternative<FunctionCall>(*obj.obj_ptr)) {
        return std::nullopt;
    }
    const auto & f = std::get<FunctionCall>(*obj.obj_ptr);

    // This is not a free function
    if (!std::holds_alternative<std::monostate>(f.holder.object())) {
        return std::nullopt;
    }

    if (!all_args_reduced(f.pos_args, f.kw_args)) {
        return std::nullopt;
    }

    std::optional<Instruction> i = std::nullopt;

    if (f.name == "rel_eq") {
        i = lower_eq(f);
    } else if (f.name == "rel_ne") {
        i = lower_ne(f);
    } else if (f.name == "unary_not") {
        i = lower_not(f);
    } else if (f.name == "unary_neg") {
        i = lower_neg(f);
    } else if (f.name == "assert") {
        i = lower_assert(f);
    } else if (f.name == "message" || f.name == "warning" || f.name == "error") {
        i = lower_messages(f);
    } else if (f.name == "include_directories") {
        i = lower_include_dirs(f, pstate);
    } else if (f.name == "files") {
        i = lower_files(f, pstate);
    } else if (f.name == "custom_target") {
        i = lower_custom_target(f, pstate);
    } else if (f.name == "executable") {
        i = lower_build_target<Executable>(f, pstate);
    } else if (f.name == "static_library") {
        i = lower_build_target<StaticLibrary>(f, pstate);
    } else if (f.name == "declare_dependency") {
        i = lower_declare_dependency(f, pstate);
    } else if (f.name == "vcs_tag") {
        i = lower_vcs_tag(f, pstate);
    } else if (f.name == "test") {
        i = lower_test(f, pstate);
    } else if (f.name == "add_project_arguments") {
        i = lower_add_arguments(f, ArgumentScope::project_comp, pstate);
    } else if (f.name == "add_project_link_arguments") {
        i = lower_add_arguments(f, ArgumentScope::project_link, pstate);
    } else if (f.name == "add_global_arguments") {
        i = lower_add_arguments(f, ArgumentScope::global_comp, pstate);
    } else if (f.name == "add_global_link_arguments") {
        i = lower_add_arguments(f, ArgumentScope::global_link, pstate);
    } else if (f.name == "disabler") {
        i = Disabler{};
    } else if (f.name == "find_program" || f.name == "dependency") {
        // These are handled elsewhere
        return std::nullopt;
    }

    if (!i) {
        throw Util::Exceptions::MesonException("Unexpected function name:" + f.name);
    }

    i.value().var = obj.var;
    return i;
}

bool all_args_reduced(const std::vector<Instruction> & pos_args,
                      const std::unordered_map<std::string, Instruction> & kw_args) {
    return std::all_of(pos_args.begin(), pos_args.end(),
                       [](auto && p) { return holds_reduced(p); }) &&
           std::all_of(kw_args.begin(), kw_args.end(),
                       [](auto && kw) { return holds_reduced(kw.second); });
}

void lower_project(std::shared_ptr<CFGNode> block, State::Persistant & pstate) {
    const auto & obj = block->block->instructions.front();

    if (!std::holds_alternative<FunctionCall>(*obj.obj_ptr)) {
        throw Util::Exceptions::MesonException{
            "First non-whitespace, non-comment must be a call to project()"};
    }
    const auto & f = std::get<FunctionCall>(*obj.obj_ptr);

    if (f.name != "project") {
        throw Util::Exceptions::MesonException{
            "First non-whitespace, non-comment must be a call to project()"};
    }

    // This doesn't handle the listified version corretly
    if (f.pos_args.empty()) {
        throw Util::Exceptions::InvalidArguments{"project requires at least 1 argument"};
    }

    auto pos = f.pos_args.begin();

    pstate.name = std::get<String>(*pos->obj_ptr).value;
    // TODO: I don't want this in here, I'd rather have this all done in the backend, I think
    std::cout << "Project name: " << Util::Log::bold(pstate.name) << std::endl;

    const auto & langs = extract_variadic_arguments<String>(
        ++pos, f.pos_args.end(), "project: Language arguments must be strings");
    for (const auto & lang : langs) {
        const auto l = Toolchain::from_string(lang.value);

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

    auto version = extract_keyword_argument<MIR::String>(f.kw_args, "version",
                                                         "project: 'version' must be a string")
                       .value_or(MIR::String{"unknown"});
    pstate.project_version = version.value;

    // TODO: handle remaining keyword arguments

    // Remove the valid project() call so we don't accidently find it later when
    // looking for invalid function calls.
    block->block->instructions.pop_front();
}

} // namespace MIR::Passes
