// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

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

std::string printer(const Object & obj) {
    return std::visit(
        [](const auto & o) -> std::string {
            if constexpr (std::is_same_v<std::decay_t<decltype(o)>, std::monostate>) {
                return "INVALID STATE";
            } else {
                return o->print();
            }
        },
        obj);
}

std::string to_string(const DependencyType d) {
    switch (d) {
        case DependencyType::INTERNAL:
            return "INTERNAL";
    }
    assert(false); // Unreachable
}

std::string to_string(const std::vector<Object> & args) {
    std::vector<std::string> vec{};
    std::transform(args.begin(), args.end(), std::back_inserter(vec),
                   [](const Object & i) { return printer(i); });
    return join(vec);
}

std::string to_string(const std::vector<FilePtr> & args) {
    std::vector<std::string> vec{};
    std::transform(args.begin(), args.end(), std::back_inserter(vec),
                   [](const FilePtr & i) { return i->print(); });
    return join(vec);
}

std::string to_string(const std::unordered_map<std::string, Object> & map) {
    std::vector<std::string> args{};
    std::transform(map.begin(), map.end(), std::back_inserter(args),
                   [](const std::pair<const std::string, const Object> & p) {
                       return p.first + " : " + printer(p.second);
                   });
    return join(args);
}

} // namespace

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

bool Phi::is_reduced() const { return false; }

std::string Phi::print() const {
    return "Phi { left = " + to_string(left) + "; right = " + to_string(right) + " }";
}

CFGNode::CFGNode() : block{std::make_unique<BasicBlock>()}, index{++bb_index} {};

bool CFGNode::operator<(const CFGNode & other) const { return index < other.index; }

bool CFGComparitor::operator()(const std::weak_ptr<CFGNode> lhs,
                               const std::weak_ptr<CFGNode> rhs) const {
    return *lhs.lock() < *rhs.lock();
}

bool CFGComparitor::operator()(const std::shared_ptr<CFGNode> & lhs,
                               const std::shared_ptr<CFGNode> & rhs) const {
    return *lhs < *rhs;
}

CFG::CFG(std::shared_ptr<CFGNode> n) : root{n} {};

Compiler::Compiler(std::shared_ptr<MIR::Toolchain::Toolchain> tc) : toolchain{std::move(tc)} {};

bool Compiler::is_reduced() const { return true; }

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

bool File::is_reduced() const { return true; }

std::string File::print() const {
    return "File { path = " + std::string{relative_to_source_dir()} +
           "; is_built = " + to_string(is_built()) + " }";
}

std::ostream & operator<<(std::ostream & os, const File & f) {
    return os << (f.is_built() ? f.build_root : f.source_root) / f.subdir / f.get_name();
}

Executable::Executable(std::string name_, std::vector<Object> srcs, const Machines::Machine & m,
                       fs::path sdir, ArgMap args, std::vector<StaticLinkage> s_link)
    : name{std::move(name_)}, sources{std::move(srcs)}, machine{m}, subdir{std::move(sdir)},
      arguments{std::move(args)}, link_static{std::move(s_link)} {};

std::string Executable::output() const { return name; }

bool Executable::is_reduced() const { return true; }

std::string Executable::print() const {
    // TODO: arguments, static_linkage
    return "Executable { name = " + name + "; machine = " + to_string(machine) +
           "; subdir = " + std::string{subdir} + "; sources = " + to_string(sources) + " }";
}

StaticLibrary::StaticLibrary(std::string name_, std::vector<Object> srcs,
                             const Machines::Machine & m, fs::path sdir, ArgMap args,
                             std::vector<StaticLinkage> s_link)
    : name{std::move(name_)}, sources{std::move(srcs)}, machine{m}, subdir{std::move(sdir)},
      arguments{std::move(args)}, link_static{std::move(s_link)} {};

bool StaticLibrary::is_reduced() const { return true; }

std::string StaticLibrary::print() const {
    // TODO: arguments, static_linkage
    return "StaticLibrary { name = " + name + "; machine = " + to_string(machine) +
           "; subdir = " + std::string{subdir} + "; sources = " + to_string(sources) + " }";
}

std::string StaticLibrary::output() const { return name + ".a"; }

IncludeDirectories::IncludeDirectories(std::vector<std::string> d, const bool & s)
    : directories{std::move(d)}, is_system{s} {};

bool IncludeDirectories::is_reduced() const { return true; }

std::string IncludeDirectories::print() const {
    return "IncludeDirectories { directories = " + join(directories) +
           "; is_system = " + to_string(is_system) + " }";
}

Message::Message(const MessageLevel & l, std::string m) : level{l}, message{std::move(m)} {};

bool Message::is_reduced() const { return true; }

std::string Message::print() const {
    return "Message { level = " + to_string(level) + "; message = " + message + " }";
}

Program::Program(std::string n, const Machines::Machine & m, fs::path p)
    : name{std::move(n)}, for_machine{m}, path{std::move(p)} {};

bool Program::is_reduced() const { return true; }

std::string Program::print() const {
    return "Program { name = " + name + "; machine = " + to_string(for_machine) +
           "; path = " + std::string{path} + " }";
}

bool Program::found() const { return path != ""; }

FunctionCall::FunctionCall(std::string _name, std::vector<Object> && _pos,
                           std::unordered_map<std::string, Object> && _kw,
                           std::filesystem::path _sd)
    : name{std::move(_name)}, pos_args{std::move(_pos)}, kw_args{std::move(_kw)},
      holder{std::nullopt}, source_dir{std::move(_sd)} {};

FunctionCall::FunctionCall(std::string _name, std::vector<Object> && _pos,
                           std::filesystem::path _sd)
    : name{std::move(_name)}, pos_args{std::move(_pos)}, holder{std::nullopt},
      source_dir{std::move(_sd)} {};

bool FunctionCall::is_reduced() const { return false; }

std::string FunctionCall::print() const {
    std::stringstream ss{};
    ss << "FunctionCall { name = { " + name + " };";
    ss << " holder = {";
    if (holder) {
        ss << " " << printer(holder.value());
    }
    ss << " };"
       << " args = { " << to_string(pos_args) << " };"
       << " kwargs = { " << to_string(kw_args) << " };"
       << " };";

    return ss.str();
}

String::String(std::string f) : value{std::move(f)} {};

bool String::is_reduced() const { return true; }

std::string String::print() const { return "'" + value + "'"; }

bool String::operator!=(const String & o) const { return value != o.value; }

bool String::operator==(const String & o) const { return value == o.value; }

Boolean::Boolean(const bool & f) : value{f} {};

bool Boolean::is_reduced() const { return true; }

std::string Boolean::print() const { return value ? "true" : "false"; }

bool Boolean::operator!=(const Boolean & o) const { return value != o.value; }
bool Boolean::operator==(const Boolean & o) const { return value == o.value; }

Number::Number(const int64_t & f) : value{f} {};

bool Number::is_reduced() const { return true; }

std::string Number::print() const { return to_string(value); }

bool Number::operator!=(const Number & o) const { return value != o.value; }
bool Number::operator==(const Number & o) const { return value == o.value; }

Identifier::Identifier(std::string s) : value{std::move(s)}, version{} {};
Identifier::Identifier(std::string s, const uint32_t & ver) : value{std::move(s)}, version{ver} {};

bool Identifier::is_reduced() const { return false; }

std::string Identifier::print() const {
    return "Identifier { value = " + value + "; version = " + to_string(version) + " }";
}

Array::Array(std::vector<Object> && a) : value{std::move(a)} {};

bool Array::is_reduced() const {
    return std::all_of(value.begin(), value.end(), [](const Object & obj) {
        return std::visit([](auto && o) { return o->is_reduced(); }, obj);
    });
}

std::string Array::print() const { return "Array { value = " + to_string(value) + " }"; }

bool Dict::is_reduced() const {
    return std::all_of(value.begin(), value.end(), [](auto && pair) {
        // TODO: need to handle key being an Object
        auto && [_, v] = pair;
        return std::visit([](auto && o) { return o->is_reduced(); }, v);
    });
}

std::string Dict::print() const { return "Dict { value = " + to_string(value) + " }"; }

CustomTarget::CustomTarget(std::string n, std::vector<Object> i, std::vector<FilePtr> o,
                           std::vector<std::string> c, fs::path s, std::vector<FilePtr> d,
                           std::optional<std::string> df)
    : name{std::move(n)}, inputs{std::move(i)}, outputs{std::move(o)}, command{std::move(c)},
      subdir{std::move(s)}, depends{std::move(d)}, depfile{std::move(df)} {};

std::string CustomTarget::print() const {
    // TODO: arguments, static_linkage
    return "CustomTarget { name = " + name + "; inputs = " + to_string(inputs) +
           "; outputs = " + to_string(outputs) + "command = " + join(command) +
           "; subdir = " + std::string{subdir} + " }";
}

bool CustomTarget::is_reduced() const { return true; }

Dependency::Dependency(std::string n, const bool & f, std::string ver,
                       std::vector<Arguments::Argument> a)
    : name{std::move(n)}, found{f}, version{std::move(ver)}, arguments{std::move(a)} {};

bool Dependency::is_reduced() const { return true; }

std::string Dependency::print() const {
    std::vector<std::string> args{};
    std::transform(arguments.begin(), arguments.end(), std::back_inserter(args),
                   [](const Arguments::Argument & a) { return a.value(); });
    // TODO: it would be nice to have a more complete argument, transforming type as well

    return "Dependency { name = " + name + "; found = " + to_string(found) +
           "; version = " + version + "; arguments = " + join(args) +
           "; type = " + to_string(type) + " }";
}

Test::Test(std::string n, Callable exe, std::vector<std::variant<StringPtr, FilePtr>> args,
           bool xfail)
    : name{std::move(n)}, executable{std::move(exe)}, arguments{std::move(args)},
      should_fail{xfail} {};

bool Test::is_reduced() const { return true; }

std::string Test::print() const {
    return "Test { name = " + name +
           "; executable = " + std::visit([](auto && arg) { return arg->print(); }, executable) +
           "; should_fail = " + (should_fail ? "true" : "false") + " }";
}

AddArguments::AddArguments(ArgMap && args, bool global)
    : arguments{std::move(args)}, is_global{global} {};

bool AddArguments::is_reduced() const { return true; }

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

Jump::Jump(std::shared_ptr<CFGNode> t) : target{std::move(t)} {};
Jump::Jump(std::shared_ptr<CFGNode> t, Object p) : target{std::move(t)}, predicate{std::move(p)} {};

// In a language that cannot be deterministically transpiled, this could be
// reduced, but for Meson it is not fully reduced, as there should be no control
// flow left when leaving MIR
bool Jump::is_reduced() const { return false; }

std::string Jump::print() const {
    return "jump { target = { " + std::to_string(target->index) + " }; predicate = { " +
           (!predicate ? "always" : printer(predicate.value())) + " } }";
}

Branch::Branch() = default;

// In a language that cannot be deterministically transpiled, this could be
// reduced, but for Meson it is not fully reduced, as there should be no control
// flow left when leaving MIR
bool Branch::is_reduced() const { return false; }

std::string Branch::print() const {
    std::stringstream ss;
    ss << "branch = { ";
    for (auto && [i, dest] : branches) {
        ss << "branch " << printer(i) << " = { " << dest->index << " }, ";
    }
    ss << " }";
    return ss.str();
}

Disabler::Disabler() = default;

std::string Disabler::print() const { return "disabler { }"; }

bool Disabler::is_reduced() const { return true; }

std::string Meson::print() const { return "Meson { }"; }

bool Meson::is_reduced() const { return true; }

void link_nodes(std::shared_ptr<CFGNode> predecessor, std::shared_ptr<CFGNode> successor) {
    successor->predecessors.emplace(predecessor);
    predecessor->successors.emplace(successor);
}

void unlink_nodes(std::shared_ptr<CFGNode> predecessor, std::shared_ptr<CFGNode> successor,
                  bool recursive) {
    // If the successor only has one parent it will be freed. When this happens
    // any blocks that consider it a predecessor will an expired weak_ptr.
    //
    // In order to avoid that situation, we look at the successor, and if it has
    // only one predecessor (us), then we recursively unlink it down the chain
    // as long as that is true.
    if (recursive && successor->predecessors.size() == 1) {
        while (!successor->successors.empty()) {
            unlink_nodes(successor, *successor->successors.begin());
        }
    }

    successor->predecessors.erase(predecessor);
    predecessor->successors.erase(successor);
}

void set_var(const Object & src, Object & dest) {
    const Variable & var = std::visit(VariableGetter{}, src);
    std::visit(VariableSetter{var}, dest);
}

} // namespace MIR
