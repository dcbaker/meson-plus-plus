// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "mir.hpp"
#include "exceptions.hpp"

namespace MIR {

namespace {

static uint32_t bb_index = 0;

}

Phi::Phi() : left{}, right{} {};
Phi::Phi(const uint32_t & l, const uint32_t & r, const Variable & v) : left{l}, right{r}, var{v} {};

bool Phi::operator==(const Phi & other) const {
    return var.name == other.var.name && left == other.left && right == other.right;
}

bool Phi::operator<(const Phi & other) const {
    return var.name < other.var.name && left < other.left && right < other.right;
}

BasicBlock::BasicBlock() : instructions{}, next{std::monostate{}}, parents{}, index{++bb_index} {};

BasicBlock::BasicBlock(std::unique_ptr<Condition> && con)
    : instructions{}, next{std::move(con)}, parents{}, index{++bb_index} {};

bool BasicBlock::operator<(const BasicBlock & other) const { return index < other.index; }

bool BBComparitor::operator()(const BasicBlock * lhs, const BasicBlock * rhs) const {
    return *lhs < *rhs;
}

Condition::Condition(Object && o)
    : condition{std::move(o)}, if_true{std::make_shared<BasicBlock>()}, if_false{nullptr} {};

Condition::Condition(Object && o, std::shared_ptr<BasicBlock> s)
    : condition{std::move(o)}, if_true{std::move(s)}, if_false{nullptr} {};

Compiler::Compiler(std::shared_ptr<MIR::Toolchain::Toolchain> tc) : toolchain{std::move(tc)} {};

const Object Compiler::get_id(const std::vector<Object> & args,
                              const std::unordered_map<std::string, Object> & kwargs) const {
    if (!args.empty()) {
        throw Util::Exceptions::InvalidArguments(
            "compiler.get_id(): takes no positional arguments");
    }
    if (!kwargs.empty()) {
        throw Util::Exceptions::InvalidArguments("compiler.get_id(): takes no keyword arguments");
    }

    return std::make_shared<String>(toolchain->compiler->id());
};

Variable::Variable() : name{}, version{0} {};
Variable::Variable(std::string n) : name{std::move(n)}, version{0} {};
Variable::Variable(std::string n, const uint32_t & v) : name{std::move(n)}, version{v} {};
Variable::Variable(const Variable & v) : name{v.name}, version{v.version} {};

Variable::operator bool() const { return !name.empty(); };

bool Variable::operator<(const Variable & other) const {
    return name < other.name || (name == other.name && version < other.version);
}

bool Variable::operator==(const Variable & other) const {
    return name == other.name && version == other.version;
}

File::File(std::string name_, fs::path sdir, const bool & built_, fs::path sr_, fs::path br_)
    : name{std::move(name_)}, subdir{std::move(sdir)}, built{built_}, source_root{std::move(sr_)},
      build_root{std::move(br_)}, var{} {};
File::File(std::string name_, fs::path sdir, const bool & built_, fs::path sr_, fs::path br_,
           const Variable & v)
    : name{std::move(name_)}, subdir{std::move(sdir)}, built{built_}, source_root{std::move(sr_)},
      build_root{std::move(br_)}, var{v} {};

const bool File::is_built() const { return built; }

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
    } else {
        return subdir / name;
    }
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
    } else {
        return subdir / name;
    }
}

bool File::operator==(const File & f) const {
    return subdir / name == f.subdir / f.name && built == f.built;
}

bool File::operator!=(const File & f) const {
    return subdir / name != f.subdir / f.name || built != f.built;
}

std::ostream & operator<<(std::ostream & os, const File & f) {
    return os << (f.is_built() ? f.build_root : f.source_root) / f.subdir / f.get_name();
}

Executable::Executable(std::string name_, std::vector<Source> srcs, const Machines::Machine & m,
                       fs::path sdir, ArgMap args, std::vector<StaticLinkage> s_link,
                       const Variable & v)
    : name{std::move(name_)}, sources{std::move(srcs)}, machine{m}, subdir{std::move(sdir)},
      arguments{std::move(args)}, link_static{std::move(s_link)}, var{v} {};

std::string Executable::output() const { return name; }

StaticLibrary::StaticLibrary(std::string name_, std::vector<Source> srcs,
                             const Machines::Machine & m, fs::path sdir, ArgMap args,
                             std::vector<StaticLinkage> s_link, const Variable & v)
    : name{std::move(name_)}, sources{std::move(srcs)}, machine{m}, subdir{std::move(sdir)},
      arguments{std::move(args)}, link_static{std::move(s_link)}, var{v} {};

std::string StaticLibrary::output() const { return name + ".a"; }

IncludeDirectories::IncludeDirectories(std::vector<std::string> d, const bool & s,
                                       const Variable & v)
    : directories{std::move(d)}, is_system{s}, var{v} {};

Message::Message(const MessageLevel & l, std::string m) : level{l}, message{std::move(m)} {};

Program::Program(std::string n, const Machines::Machine & m, fs::path p)
    : name{std::move(n)}, for_machine{m}, path{std::move(p)} {};
Program::Program(std::string n, const Machines::Machine & m, fs::path p, const Variable & v)
    : name{std::move(n)}, for_machine{m}, path{std::move(p)}, var{v} {};

bool Program::found() const { return path != ""; }

FunctionCall::FunctionCall(std::string _name, std::vector<Object> && _pos,
                           std::unordered_map<std::string, Object> && _kw,
                           std::filesystem::path _sd)
    : name{std::move(_name)}, pos_args{std::move(_pos)}, kw_args{std::move(_kw)},
      holder{std::nullopt}, source_dir{std::move(_sd)}, var{} {};

FunctionCall::FunctionCall(std::string _name, std::vector<Object> && _pos,
                           std::filesystem::path _sd)
    : name{std::move(_name)}, pos_args{std::move(_pos)}, kw_args{}, holder{std::nullopt},
      source_dir{std::move(_sd)}, var{} {};

String::String(std::string f) : value{std::move(f)}, var{} {};

bool String::operator!=(const String & o) const { return value != o.value; }

bool String::operator==(const String & o) const { return value == o.value; }

Boolean::Boolean(const bool & f) : value{f}, var{} {};
Boolean::Boolean(const bool & f, const Variable & v) : value{f}, var{v} {};

bool Boolean::operator!=(const Boolean & o) const { return value != o.value; }
bool Boolean::operator==(const Boolean & o) const { return value == o.value; }

Number::Number(const int64_t & f) : value{f}, var{} {};

bool Number::operator!=(const Number & o) const { return value != o.value; }
bool Number::operator==(const Number & o) const { return value == o.value; }

Identifier::Identifier(std::string s) : value{std::move(s)}, version{}, var{} {};
Identifier::Identifier(std::string s, const uint32_t & ver, Variable && v)
    : value{std::move(s)}, version{ver}, var{std::move(v)} {};

Array::Array() : value{}, var{} {};
Array::Array(std::vector<Object> && a) : value{std::move(a)} {};

Dict::Dict() : value{}, var{} {};

CustomTarget::CustomTarget(std::string n, std::vector<Source> i, std::vector<File> o,
                           std::vector<std::string> c, fs::path s, const Variable & v)
    : name{std::move(n)}, inputs{std::move(i)}, outputs{std::move(o)}, command{std::move(c)},
      subdir{std::move(s)}, var{v} {};

Dependency::Dependency(std::string n, const bool & f, std::string ver,
                       std::vector<Arguments::Argument> a, const Variable & v)
    : name{std::move(n)}, found{f}, version{std::move(ver)}, arguments{std::move(a)}, var{v} {};

} // namespace MIR
