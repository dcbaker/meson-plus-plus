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
    : condition{std::move(o)}, if_true{s}, if_false{nullptr} {};

Compiler::Compiler(const std::shared_ptr<MIR::Toolchain::Toolchain> & tc) : toolchain{tc} {};

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
Variable::Variable(const std::string & n) : name{n}, version{0} {};
Variable::Variable(const std::string & n, const uint32_t & v) : name{n}, version{v} {};
Variable::Variable(const Variable & v) : name{v.name}, version{v.version} {};

Variable::operator bool() const { return !name.empty(); };

bool Variable::operator<(const Variable & other) const {
    return name < other.name || (name == other.name && version < other.version);
}

bool Variable::operator==(const Variable & other) const {
    return name == other.name && version == other.version;
}

File::File(const std::string & name_, const fs::path & sdir, const bool & built_,
           const fs::path & sr_, const fs::path & br_)
    : name{name_}, subdir{sdir}, built{built_}, source_root{sr_}, build_root{br_}, var{} {};
File::File(const std::string & name_, const fs::path & sdir, const bool & built_,
           const fs::path & sr_, const fs::path & br_, const Variable & v)
    : name{name_}, subdir{sdir}, built{built_}, source_root{sr_}, build_root{br_}, var{v} {};

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

Executable::Executable(const std::string & name_, const std::vector<Source> & srcs,
                       const Machines::Machine & m, const fs::path & sdir, const ArgMap & args,
                       const std::vector<StaticLinkage> s_link, const Variable & v)
    : name{name_}, sources{srcs}, machine{m}, subdir{sdir}, arguments{args},
      link_static{s_link}, var{v} {};

std::string Executable::output() const { return name; }

StaticLibrary::StaticLibrary(const std::string & name_, const std::vector<Source> & srcs,
                             const Machines::Machine & m, const fs::path & sdir,
                             const ArgMap & args, const std::vector<StaticLinkage> s_link,
                             const Variable & v)
    : name{name_}, sources{srcs}, machine{m}, subdir{sdir}, arguments{args},
      link_static{s_link}, var{v} {};

std::string StaticLibrary::output() const { return name + ".a"; }

IncludeDirectories::IncludeDirectories(const std::vector<std::string> & d, const bool & s,
                                       const Variable & v)
    : directories{d}, is_system{s}, var{v} {};

Message::Message(const MessageLevel & l, const std::string & m) : level{l}, message{m} {};

Program::Program(const std::string & n, const Machines::Machine & m, const fs::path & p)
    : name{n}, for_machine{m}, path{p} {};
Program::Program(const std::string & n, const Machines::Machine & m, const fs::path & p,
                 const Variable & v)
    : name{n}, for_machine{m}, path{p}, var{v} {};

bool Program::found() const { return path != ""; }

FunctionCall::FunctionCall(const std::string & _name, std::vector<Object> && _pos,
                           std::unordered_map<std::string, Object> && _kw,
                           const std::filesystem::path & _sd)
    : name{_name}, pos_args{std::move(_pos)}, kw_args{std::move(_kw)}, holder{std::nullopt},
      source_dir{_sd}, var{} {};

FunctionCall::FunctionCall(const std::string & _name, std::vector<Object> && _pos,
                           const std::filesystem::path & _sd)
    : name{_name}, pos_args{std::move(_pos)}, kw_args{}, holder{std::nullopt},
      source_dir{_sd}, var{} {};

String::String(const std::string & f) : value{f}, var{} {};

Boolean::Boolean(const bool & f) : value{f}, var{} {};
Boolean::Boolean(const bool & f, const Variable & v) : value{f}, var{v} {};

Number::Number(const int64_t & f) : value{f}, var{} {};

Identifier::Identifier(const std::string & s) : value{s}, version{}, var{} {};
Identifier::Identifier(const std::string & s, const uint32_t & ver, Variable && v)
    : value{s}, version{ver}, var{std::move(v)} {};

Array::Array() : value{}, var{} {};
Array::Array(std::vector<Object> && a) : value{std::move(a)} {};

Dict::Dict() : value{}, var{} {};

CustomTarget::CustomTarget(const std::string & n, const std::vector<Source> & i,
                           const std::vector<File> & o, const std::vector<std::string> & c,
                           const fs::path & s, const Variable & v)
    : name{n}, inputs{i}, outputs{o}, command{c}, subdir{s}, var{v} {};

} // namespace MIR
