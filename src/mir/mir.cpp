// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <algorithm>
#include <iterator>
#include <utility>

#include "exceptions.hpp"
#include "mir.hpp"

namespace MIR {

namespace {

uint32_t bb_index = 0;

std::string join(const std::vector<std::string> & vec, const char * delim = ", ") {
    std::stringstream stream{};
    std::copy(vec.begin(), vec.end(), std::ostream_iterator<std::string>(stream, delim));
    return stream.str();
}

using Machines::to_string;
using std::to_string;

std::string to_string(const MessageLevel l) {
    switch (l) {
        case MessageLevel::DEBUG:
            return "DEBUG";
        case MessageLevel::MESSAGE:
            return "MESSAGE";
        case MessageLevel::WARN:
            return "WARN";
        case MessageLevel::ERROR:
            return "ERROR";
    }
    assert(false); // Unreachable
}

std::string to_string(const DependencyType d) {
    switch (d) {
        case DependencyType::INTERNAL:
            return "INTERNAL";
    }
    assert(false); // Unreachable
}

std::string to_string(const std::vector<Instruction> & args) {
    std::vector<std::string> vec{};
    std::transform(args.begin(), args.end(), std::back_inserter(vec),
                   [](const Instruction & i) { return i.print(); });
    return join(vec);
}

std::string to_string(const std::vector<File> & args) {
    std::vector<std::string> vec{};
    std::transform(args.begin(), args.end(), std::back_inserter(vec),
                   [](const File & i) { return i.print(); });
    return join(vec);
}

std::string to_string(const std::unordered_map<std::string, Instruction> & map) {
    std::vector<std::string> args{};
    std::transform(map.begin(), map.end(), std::back_inserter(args),
                   [](const std::pair<const std::string, const Instruction> & p) {
                       return p.first + " : " + p.second.print();
                   });
    return join(args);
}

} // namespace

Instruction::Instruction() : obj_ptr{std::make_shared<Object>(std::monostate{})} {};
Instruction::Instruction(std::shared_ptr<Object> ptr) : obj_ptr{std::move(std::move(ptr))} {};
Instruction::Instruction(const Object & obj) : obj_ptr{std::make_shared<Object>(obj)} {};
Instruction::Instruction(Object obj, const Variable & var_)
    : obj_ptr{std::make_shared<Object>(std::move(obj))}, var{var_} {};
Instruction::Instruction(Boolean val) : obj_ptr{std::make_shared<Object>(val)} {};
Instruction::Instruction(Number val) : obj_ptr{std::make_shared<Object>(val)} {};
Instruction::Instruction(String val) : obj_ptr{std::make_shared<Object>(std::move(val))} {};
Instruction::Instruction(FunctionCall val) : obj_ptr{std::make_shared<Object>(std::move(val))} {};
Instruction::Instruction(Identifier val) : obj_ptr{std::make_shared<Object>(std::move(val))} {};
Instruction::Instruction(Array val) : obj_ptr{std::make_shared<Object>(std::move(val))} {};
Instruction::Instruction(Dict val) : obj_ptr{std::make_shared<Object>(std::move(val))} {};
Instruction::Instruction(File val) : obj_ptr{std::make_shared<Object>(std::move(val))} {};
Instruction::Instruction(IncludeDirectories val)
    : obj_ptr{std::make_shared<Object>(std::move(val))} {};
Instruction::Instruction(Message val) : obj_ptr{std::make_shared<Object>(std::move(val))} {};
Instruction::Instruction(Empty val) : obj_ptr{std::make_shared<Object>(val)} {};
Instruction::Instruction(Dependency val) : obj_ptr{std::make_shared<Object>(std::move(val))} {};
Instruction::Instruction(CustomTarget val) : obj_ptr{std::make_shared<Object>(std::move(val))} {};
Instruction::Instruction(StaticLibrary val) : obj_ptr{std::make_shared<Object>(std::move(val))} {};
Instruction::Instruction(Executable val) : obj_ptr{std::make_shared<Object>(std::move(val))} {};
Instruction::Instruction(Phi val) : obj_ptr{std::make_shared<Object>(val)} {};
Instruction::Instruction(Program val) : obj_ptr{std::make_shared<Object>(std::move(val))} {};
Instruction::Instruction(Test val) : obj_ptr{std::make_shared<Object>(std::move(val))} {};
Instruction::Instruction(AddArguments val) : obj_ptr{std::make_shared<Object>(std::move(val))} {};

std::string Instruction::print() const {
    const std::string i = std::visit(
        [](auto && i) {
            using T = std::decay_t<decltype(i)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                return std::string{"Empty {}"};
            } else {
                return i.print();
            }
        },
        *obj_ptr);
    return "Instruction { object = " + i + "; var = " + var.print() + " }";
}

const Object & Instruction::object() const { return *obj_ptr; }

Phi::Phi() : left{}, right{} {};
Phi::Phi(const uint32_t & l, const uint32_t & r)
    : left{l}, right{r} {}; // NOLINT(bugprone-easily-swappable-parameters)

bool Phi::operator==(const Phi & other) const {
    // TODO: need to handle name == name
    return left == other.left && right == other.right;
}

bool Phi::operator<(const Phi & other) const {
    // TODO: need to handle name < name
    return left < other.left && right < other.right;
}

std::string Phi::print() const {
    return "Phi { left = " + to_string(left) + "; right = " + to_string(right) + " }";
}

BasicBlock::BasicBlock() : next{std::monostate{}}, index{++bb_index} {};

BasicBlock::BasicBlock(std::unique_ptr<Condition> && con)
    : next{std::move(con)}, index{++bb_index} {};

bool BasicBlock::operator<(const BasicBlock & other) const { return index < other.index; }

bool BBComparitor::operator()(const BasicBlock * lhs, const BasicBlock * rhs) const {
    return *lhs < *rhs;
}

Condition::Condition(Instruction && o)
    : condition{std::move(o)}, if_true{std::make_shared<BasicBlock>()}, if_false{nullptr} {};

Condition::Condition(Instruction && o, std::shared_ptr<BasicBlock> s)
    : condition{std::move(o)}, if_true{std::move(s)}, if_false{nullptr} {};

Compiler::Compiler(std::shared_ptr<MIR::Toolchain::Toolchain> tc) : toolchain{std::move(tc)} {};

Instruction Compiler::get_id(const std::vector<Instruction> & args,
                             const std::unordered_map<std::string, Instruction> & kwargs) const {
    if (!args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "compiler.get_id(): takes no positional arguments");
    }
    if (!kwargs.empty()) {
        throw Util::Exceptions::InvalidArguments("compiler.get_id(): takes no keyword arguments");
    }

    return std::make_shared<Object>(String{toolchain->compiler->id()});
};

std::string Compiler::print() const {
    return "Compiler { language = " + toolchain->compiler->language() +
           "; id = " + toolchain->compiler->id() + " }";
}

Variable::Variable() : gvn{0} {};
Variable::Variable(std::string n) : name{std::move(n)}, gvn{0} {};
Variable::Variable(std::string n, const uint32_t & v) : name{std::move(n)}, gvn{v} {};

Variable::operator bool() const { return !name.empty(); };

bool Variable::operator<(const Variable & other) const {
    return name < other.name || (name == other.name && gvn < other.gvn);
}

bool Variable::operator==(const Variable & other) const {
    return name == other.name && gvn == other.gvn;
}

std::string Variable::print() const {
    return "Variable { name = " + name + "; gvn = " + to_string(gvn) + " }";
}

File::File(std::string name_, fs::path sdir, const bool & built_, fs::path sr_, fs::path br_)
    : name{std::move(name_)}, subdir{std::move(sdir)}, built{built_}, source_root{std::move(sr_)},
      build_root{std::move(br_)} {};

bool File::is_built() const { return built; }

std::string File::get_name() const { return name; }

std::filesystem::path File::relative_to_source_dir() const {
    if (built) {
        std::error_code ec{};
        auto p = fs::relative(build_root / subdir / name, source_root / subdir, ec);
        if (ec) {
            // TODO: better error handling
            throw Util::Exceptions::MesonException{"Failed to create relative path"};
        }
        return p;
    }
    return subdir / name;
}

std::filesystem::path File::relative_to_build_dir() const {
    if (!built) {
        std::error_code ec{};
        auto p = fs::relative(source_root / subdir / name, build_root / subdir, ec);
        if (ec) {
            // TODO: better error handling
            throw Util::Exceptions::MesonException{"Failed to create relative path"};
        }
        return p;
    }
    return subdir / name;
}

bool File::operator==(const File & f) const {
    return subdir / name == f.subdir / f.name && built == f.built;
}

bool File::operator!=(const File & f) const {
    return subdir / name != f.subdir / f.name || built != f.built;
}

std::string File::print() const {
    return "File { path = " + std::string{relative_to_source_dir()} +
           "; is_built = " + to_string(is_built()) + " }";
}

std::ostream & operator<<(std::ostream & os, const File & f) {
    return os << (f.is_built() ? f.build_root : f.source_root) / f.subdir / f.get_name();
}

Executable::Executable(std::string name_, std::vector<Instruction> srcs,
                       const Machines::Machine & m, fs::path sdir, ArgMap args,
                       std::vector<StaticLinkage> s_link)
    : name{std::move(name_)}, sources{std::move(srcs)}, machine{m}, subdir{std::move(sdir)},
      arguments{std::move(args)}, link_static{std::move(s_link)} {};

std::string Executable::output() const { return name; }

std::string Executable::print() const {
    // TODO: arguments, static_linkage
    return "Executable { name = " + name + "; machine = " + to_string(machine) +
           "; subdir = " + std::string{subdir} + "; sources = " + to_string(sources) + " }";
}

StaticLibrary::StaticLibrary(std::string name_, std::vector<Instruction> srcs,
                             const Machines::Machine & m, fs::path sdir, ArgMap args,
                             std::vector<StaticLinkage> s_link)
    : name{std::move(name_)}, sources{std::move(srcs)}, machine{m}, subdir{std::move(sdir)},
      arguments{std::move(args)}, link_static{std::move(s_link)} {};

std::string StaticLibrary::print() const {
    // TODO: arguments, static_linkage
    return "StaticLibrary { name = " + name + "; machine = " + to_string(machine) +
           "; subdir = " + std::string{subdir} + "; sources = " + to_string(sources) + " }";
}

std::string StaticLibrary::output() const { return name + ".a"; }

IncludeDirectories::IncludeDirectories(std::vector<std::string> d, const bool & s)
    : directories{std::move(d)}, is_system{s} {};

std::string IncludeDirectories::print() const {
    return "IncludeDirectories { directories = " + join(directories) +
           "; is_system = " + to_string(is_system) + " }";
}

Message::Message(const MessageLevel & l, std::string m) : level{l}, message{std::move(m)} {};

std::string Message::print() const {
    return "Message { level = " + to_string(level) + "; message = " + message + " }";
}

Program::Program(std::string n, const Machines::Machine & m, fs::path p)
    : name{std::move(n)}, for_machine{m}, path{std::move(p)} {};

std::string Program::print() const {
    return "Program { name = " + name + "; machine = " + to_string(for_machine) +
           "; path = " + std::string{path} + " }";
}

bool Program::found() const { return path != ""; }

std::string Empty::print() const { return "Empty {}"; }

FunctionCall::FunctionCall(std::string _name, std::vector<Instruction> && _pos,
                           std::unordered_map<std::string, Instruction> && _kw,
                           std::filesystem::path _sd)
    : name{std::move(_name)}, pos_args{std::move(_pos)}, kw_args{std::move(_kw)},
      holder{std::make_shared<Object>(std::monostate{})}, source_dir{std::move(_sd)} {};

FunctionCall::FunctionCall(std::string _name, std::vector<Instruction> && _pos,
                           std::filesystem::path _sd)
    : name{std::move(_name)}, pos_args{std::move(_pos)},
      holder{std::make_shared<Object>(std::monostate{})}, source_dir{std::move(_sd)} {};

std::string FunctionCall::print() const {
    return "FunctionCall { name = " + name + "; holder = { " + holder.print() + " }; args = { " +
           to_string(pos_args) + " }; kwargs = { " + to_string(kw_args) + " } }";
}

String::String(std::string f) : value{std::move(f)} {};

std::string String::print() const { return "'" + value + "'"; }

bool String::operator!=(const String & o) const { return value != o.value; }

bool String::operator==(const String & o) const { return value == o.value; }

Boolean::Boolean(const bool & f) : value{f} {};

std::string Boolean::print() const { return value ? "true" : "false"; }

bool Boolean::operator!=(const Boolean & o) const { return value != o.value; }
bool Boolean::operator==(const Boolean & o) const { return value == o.value; }

Number::Number(const int64_t & f) : value{f} {};

std::string Number::print() const { return to_string(value); }

bool Number::operator!=(const Number & o) const { return value != o.value; }
bool Number::operator==(const Number & o) const { return value == o.value; }

Identifier::Identifier(std::string s) : value{std::move(s)}, version{} {};
Identifier::Identifier(std::string s, const uint32_t & ver) : value{std::move(s)}, version{ver} {};

std::string Identifier::print() const {
    return "Identifier { value = " + value + "; version = " + to_string(version) + " }";
}

Array::Array(std::vector<Instruction> && a) : value{std::move(a)} {};

std::string Array::print() const { return "Array { value = " + to_string(value) + " }"; }

std::string Dict::print() const { return "Dict { value = " + to_string(value) + " }"; }

CustomTarget::CustomTarget(std::string n, std::vector<Instruction> i, std::vector<File> o,
                           std::vector<std::string> c, fs::path s, std::vector<File> d,
                           std::optional<std::string> df)
    : name{std::move(n)}, inputs{std::move(i)}, outputs{std::move(o)}, command{std::move(c)},
      subdir{std::move(s)}, depends{std::move(d)}, depfile{std::move(df)} {};

std::string CustomTarget::print() const {
    // TODO: arguments, static_linkage
    return "CustomTarget { name = " + name + "; inputs = " + to_string(inputs) +
           "; outputs = " + to_string(outputs) + "command = " + join(command) +
           "; subdir = " + std::string{subdir} + " }";
}

Dependency::Dependency(std::string n, const bool & f, std::string ver,
                       std::vector<Arguments::Argument> a)
    : name{std::move(n)}, found{f}, version{std::move(ver)}, arguments{std::move(a)} {};

std::string Dependency::print() const {
    std::vector<std::string> args{};
    std::transform(arguments.begin(), arguments.end(), std::back_inserter(args),
                   [](const Arguments::Argument & a) { return a.value(); });
    // TODO: it would be nice to have a more complete argument, transforming type as well

    return "Dependency { name = " + name + "; found = " + to_string(found) +
           "; version = " + version + "; arguments = " + join(args) +
           "; type = " + to_string(type) + " }";
}

Test::Test(std::string n, Callable exe, std::vector<std::variant<String, File>> args, bool xfail)
    : name{std::move(n)}, executable{std::move(exe)}, arguments{std::move(args)},
      should_fail{xfail} {};

std::string Test::print() const {
    return "Test { name = " + name +
           "; executable = " + std::visit([](auto && arg) { return arg.print(); }, executable) +
           "; should_fail = " + (should_fail ? "true" : "false") + " }";
}

AddArguments::AddArguments(ArgMap && args, bool global)
    : arguments{std::move(args)}, is_global{global} {};

std::string AddArguments::print() const {
    std::stringstream ss{};

    ss << "AddArguments { arguments = { ";
    for (auto && [lang, args] : arguments) {
        ss << to_string(lang) << " = { ";
        for (auto && arg : args) {
            ss << arg.print() << ", ";
        }
        ss << " },";
    }
    ss << " is_global = { " << is_global << " } }";

    return ss.str();
}

} // namespace MIR
