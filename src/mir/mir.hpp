// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

/**
 * Meson++ Mid level IR
 *
 * This IR is lossy, it doesn't contain all of the information that the AST
 * does, and is designed for running lower passes on, so we can get it closer to
 * the backend IR, removing all function calls and most varibles.
 */

#pragma once

#include <cstdint>
#include <filesystem>
#include <list>
#include <memory>
#include <optional>
#include <set>
#include <string>
#include <sys/types.h>
#include <unordered_map>
#include <variant>
#include <vector>

#include "toolchains/toolchain.hpp"

namespace fs = std::filesystem;

namespace MIR {

/**
 * Information about an object when it is stored to a variable
 *
 * At the MIR level, assignments are stored to the object, as many
 * objects have creation side effects (creating a Target, for example)
 *
 * The name will be referenced against the symbol table, along with the version
 * which is used by value numbering.
 */
class Variable {
  public:
    Variable();
    Variable(std::string n);
    Variable(std::string n, const uint32_t & v);
    Variable(const Variable & v) = default;
    Variable & operator=(const Variable & v) = default;

    std::string name;

    /// The version as used by value numbering, 0 means unset
    uint32_t gvn;

    explicit operator bool() const;
    bool operator<(const Variable &) const;
    bool operator==(const Variable &) const;

    // Print a human readable version of this Variable
    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;
};

class AddArguments;
class FunctionCall;
class String;
class Boolean;
class Number;
class Identifier;
class Array;
class Dict;
class Compiler;
class File;
class Executable;
class StaticLibrary;
class Phi;
class IncludeDirectories;
class Message;
class Program;
class CustomTarget;
class Dependency;
class Test;
class Jump;
class Branch;
class Disabler;

using AddArgumentsPtr = std::shared_ptr<AddArguments>;
using FunctionCallPtr = std::shared_ptr<FunctionCall>;
using StringPtr = std::shared_ptr<String>;
using BooleanPtr = std::shared_ptr<Boolean>;
using NumberPtr = std::shared_ptr<Number>;
using IdentifierPtr = std::shared_ptr<Identifier>;
using ArrayPtr = std::shared_ptr<Array>;
using DictPtr = std::shared_ptr<Dict>;
using CompilerPtr = std::shared_ptr<Compiler>;
using FilePtr = std::shared_ptr<File>;
using ExecutablePtr = std::shared_ptr<Executable>;
using StaticLibraryPtr = std::shared_ptr<StaticLibrary>;
using PhiPtr = std::shared_ptr<Phi>;
using IncludeDirectoriesPtr = std::shared_ptr<IncludeDirectories>;
using MessagePtr = std::shared_ptr<Message>;
using ProgramPtr = std::shared_ptr<Program>;
using CustomTargetPtr = std::shared_ptr<CustomTarget>;
using DependencyPtr = std::shared_ptr<Dependency>;
using TestPtr = std::shared_ptr<Test>;
using JumpPtr = std::shared_ptr<Jump>;
using BranchPtr = std::shared_ptr<Branch>;
using DisablerPtr = std::shared_ptr<Disabler>;

using Object =
    std::variant<AddArgumentsPtr, FunctionCallPtr, StringPtr, BooleanPtr, NumberPtr, IdentifierPtr,
                 ArrayPtr, DictPtr, CompilerPtr, FilePtr, ExecutablePtr, StaticLibraryPtr, PhiPtr,
                 IncludeDirectoriesPtr, MessagePtr, ProgramPtr, CustomTargetPtr, DependencyPtr,
                 TestPtr, JumpPtr, BranchPtr, DisablerPtr>;

using Callable = std::variant<FilePtr, ExecutablePtr, ProgramPtr>;

/**
 * Holds a File, which is a smart object point to a source
 *
 */
class File {
  public:
    File(std::string name_, fs::path sdir, const bool & built_, fs::path sr_, fs::path br_);

    /// Whether this is a built object, or a static one
    bool is_built() const;

    /// Get the name of the of the file, relative to the src dir if it's static,
    /// or the build dir if it's built
    std::string get_name() const;

    /// Get a path for this file relative to the source tree
    fs::path relative_to_source_dir() const;

    /// Get a path for this file relative to the build treeZ
    fs::path relative_to_build_dir() const;

    bool operator==(const File &) const;
    bool operator!=(const File &) const;

    /// Print a human readable version of this
    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    // For gtest
    friend std::ostream & operator<<(std::ostream & os, const File & f);

    const std::string name;
    const fs::path subdir;
    const bool built;
    const fs::path source_root;
    const fs::path build_root;

    Variable var;
};

class CustomTarget {
  public:
    CustomTarget(std::string n, std::vector<Object> i, std::vector<FilePtr> o,
                 std::vector<std::string> c, fs::path s, std::vector<FilePtr> d,
                 std::optional<std::string> df);

    const std::string name;
    const std::vector<Object> inputs;
    const std::vector<FilePtr> outputs;
    const std::vector<std::string> command;
    const fs::path subdir;
    std::vector<FilePtr> depends;
    std::optional<std::string> depfile;

    /// Print a human readable version of this
    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    Variable var;
};

using ArgMap = std::unordered_map<Toolchain::Language, std::vector<Arguments::Argument>>;

enum class StaticLinkMode {
    NORMAL,
    WHOLE,
};

class StaticLibrary;

using StaticLinkage = std::tuple<StaticLinkMode, const StaticLibraryPtr>;

class Executable {
  public:
    Executable(std::string name_, std::vector<Object> srcs, const Machines::Machine & m,
               fs::path sdir, ArgMap args, std::vector<StaticLinkage> s_link);

    /// The name of the target
    const std::string name;

    /// The sources (as files)
    const std::vector<Object> sources;

    /// Which machine is this executable to be built for?
    const Machines::Machine machine;

    /// Where is this Target defined
    const fs::path subdir;

    /**
     * Arguments for the target, sorted by langauge
     *
     * We sort these by language, as each compiled source will only recieve it's
     * per-language arguments
     */
    const ArgMap arguments;

    /// static targets to link with
    const std::vector<StaticLinkage> link_static{};

    std::string output() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    /// Print a human readable version of this
    std::string print() const;

    Variable var;
};

class StaticLibrary {
  public:
    StaticLibrary(std::string name_, std::vector<Object> srcs, const Machines::Machine & m,
                  fs::path sdir, ArgMap args, std::vector<StaticLinkage> s_link);

    /// The name of the target
    const std::string name;

    /// The sources (as files)
    const std::vector<Object> sources;

    /// Which machine is this executable to be built for?
    const Machines::Machine machine;

    /// Where is this Target defined
    const fs::path subdir;

    /**
     * Arguments for the target, sorted by langauge
     *
     * We sort these by language, as each compiled source will only recieve it's
     * per-language arguments
     */
    const ArgMap arguments;

    /// static targets to link with
    const std::vector<StaticLinkage> link_static{};

    std::string output() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    /// Print a human readable version of this
    std::string print() const;

    Variable var;
};

/**
 * A phi node
 *
 * A synthetic instruction which represents the point where to possible values
 * for a variable converge. When one strictly dominates the other then this can
 * be removed.
 */
class Phi {
  public:
    Phi();
    Phi(const uint32_t & l, const uint32_t & r);

    uint32_t left;
    uint32_t right;

    bool operator==(const Phi & other) const;
    bool operator<(const Phi & other) const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    /// Print a human readable version of this
    std::string print() const;

    Variable var;
};

class IncludeDirectories {
  public:
    IncludeDirectories(std::vector<std::string> d, const bool & s);

    const std::vector<std::string> directories;
    const bool is_system;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    /// Print a human readable version of this
    std::string print() const;

    Variable var;
};

enum class DependencyType {
    INTERNAL,
};

/**
 * A dependency object
 *
 * Holds files, arguments, etc, to apply to build targets
 */
class Dependency {
  public:
    Dependency(std::string name, const bool & found, std::string version,
               std::vector<Arguments::Argument> args);

    /// Name of the dependency
    const std::string name;

    /// whether or not the dependency is found
    const bool found;

    /// The version of the dependency
    const std::string version;

    /// Per-language compiler args
    const std::vector<Arguments::Argument> arguments;

    /// The kind of dependency this is
    const DependencyType type = DependencyType::INTERNAL;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    /// Print a human readable version of this
    std::string print() const;

    Variable var;
};

enum class MessageLevel {
    DEBUG,
    MESSAGE,
    WARN,
    ERROR,
};

class Message {
  public:
    Message(const MessageLevel & l, std::string m);

    /// What level or kind of message this is
    const MessageLevel level;

    /// The message itself
    const std::string message;

    /// Print a human readable version of this
    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    Variable var;
};

class Program {
  public:
    Program(std::string n, const Machines::Machine & m, fs::path p);

    const std::string name;
    const Machines::Machine for_machine;
    const fs::path path;

    bool found() const;

    /// Print a human readable version of this
    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    Variable var;
};

class Test {
  public:
    Test(std::string n, Callable exe, std::vector<std::variant<StringPtr, FilePtr>> args,
         bool xfail);

    const std::string name;
    const Callable executable;
    const std::vector<std::variant<StringPtr, FilePtr>> arguments;
    const bool should_fail;

    /// Print a human readable version of this
    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    Variable var;
};

/**
 * Holds a toolchain
 *
 * Called compiler as that's what it is in the Meson DSL
 */
class Compiler {
  public:
    Compiler(std::shared_ptr<MIR::Toolchain::Toolchain> tc);

    const std::shared_ptr<MIR::Toolchain::Toolchain> toolchain;

    /// Print a human readable version of this
    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    Variable var;
};

// Can be a method via an optional paramter maybe?
/// A function call object
class FunctionCall {
  public:
    FunctionCall(std::string _name, std::vector<Object> && _pos,
                 std::unordered_map<std::string, Object> && _kw, std::filesystem::path _sd);
    FunctionCall(std::string _name, std::vector<Object> && _pos, std::filesystem::path _sd);

    const std::string name;

    /// Ordered container of positional argument objects
    std::vector<Object> pos_args;

    /// Unordered container mapping keyword arguments to their values
    std::unordered_map<std::string, Object> kw_args;

    /// reference to object holding this function, it's monostate if not
    std::optional<Object> holder;

    /**
     * The directory this was called form.
     *
     * For functions that care (such as file(), and most targets()) this is
     * required to accurately map sources between the source and build dirs.
     */
    const std::filesystem::path source_dir;

    /// Print a human readable version of this
    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    Variable var;
};

class Disabler {
  public:
    Disabler();

    /// Print a human readable version of this
    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    Variable var;
};

class String {
  public:
    String(std::string f);

    bool operator==(const String &) const;
    bool operator!=(const String &) const;

    std::string value;

    /// Print a human readable version of this
    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    Variable var;
};

class Boolean {
  public:
    Boolean(const bool & f);

    bool operator==(const Boolean &) const;
    bool operator!=(const Boolean &) const;

    const bool value;

    /// Print a human readable version of this
    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    Variable var;
};

class Number {
  public:
    Number(const int64_t & f);

    bool operator==(const Number &) const;
    bool operator!=(const Number &) const;

    const int64_t value;

    /// Print a human readable version of this
    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    Variable var;
};

class Identifier {
  public:
    Identifier(std::string s);
    Identifier(std::string s, const uint32_t & ver);

    /// The name of the identifier
    const std::string value;

    /**
     * The Value numbering version
     *
     * This is only relavent in a couple of situations, namely when we've
     * replaced a phi with an identifer, and we need to be clear which version
     * this is an alias of:
     *
     *      x₄ = x₁
     *      x₅ = ϕ(x₃, x₄)
     *
     * In this case we need to know that x₄ is x₁, and not any other version.
     * however, x₄ should be promptly cleaned up by a constant folding pass,
     * removing the need to track this information long term.
     */
    uint32_t version;

    /// Print a human readable version of this
    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    Variable var;
};

class Array {
  public:
    Array() = default;
    Array(std::vector<Object> && a);
    Array(std::vector<String> && a);

    std::vector<Object> value;

    /// Print a human readable version of this
    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    Variable var;
};

class Dict {
  public:
    Dict() = default;

    // TODO: the key is allowed to be a string or an expression that evaluates
    // to a string, we need to enforce that somewhere.
    std::unordered_map<std::string, Object> value;

    /// Print a human readable version of this
    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    Variable var;
};

class AddArguments {
  public:
    AddArguments(ArgMap &&, bool global);

    /// Print a human readable version of this
    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    ArgMap arguments;
    bool is_global;

    Variable var;
};

class CFGNode;

/// @brief Jump to another block
class Jump {
  public:
    Jump(std::shared_ptr<CFGNode> t);
    Jump(std::shared_ptr<CFGNode> t, Object p);

    /// @brief Print a human readable version of this instruction
    /// @return A string representing the instructions
    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    /// @brief The block to jump to
    std::shared_ptr<CFGNode> target;

    /// @brief A potential predicate of the jump
    /// If this is a nullptr it is considered unconditional
    std::optional<Object> predicate;

    Variable var;
};

/// @brief An instruction for jumping to multiple targets based on conditions
///
/// This is mainly used as a high level if/elif/else construction,
/// but we expect to later lower it to Jumps
class Branch {
  public:
    Branch();

    std::vector<std::tuple<Object, std::shared_ptr<CFGNode>>> branches;

    std::string print() const;

    /// Is this a fully reduced object?
    bool is_reduced() const;

    Variable var;
};

class BasicBlock {
  public:
    BasicBlock() = default;

    /// The instructions in this block
    std::list<Object> instructions;

    Variable var;
};

struct CFGComparitor {
    bool operator()(const std::weak_ptr<CFGNode> lhs, const std::weak_ptr<CFGNode> rhs) const;
    bool operator()(const std::shared_ptr<CFGNode> & lhs,
                    const std::shared_ptr<CFGNode> & rhs) const;
};

/**
 * Holds a list of instructions, and optionally a condition or next block
 */
class CFGNode {
  public:
    CFGNode();

    /// @brief The block instructions
    std::unique_ptr<BasicBlock> block;

    /// All predecessors of this block
    std::set<std::weak_ptr<CFGNode>, CFGComparitor> predecessors;

    /// @brief All blocks that come after this one
    std::set<std::shared_ptr<CFGNode>, CFGComparitor> successors;

    const uint32_t index;

    bool operator<(const CFGNode &) const;
};

class CFG {
  public:
    CFG(std::shared_ptr<CFGNode> n);

    std::shared_ptr<CFGNode> root;
};

struct VariableGetter {
    Variable & operator()(const AddArgumentsPtr & o) const { return o->var; }
    Variable & operator()(const ArrayPtr & o) const { return o->var; }
    Variable & operator()(const BooleanPtr & o) const { return o->var; }
    Variable & operator()(const BranchPtr & o) const { return o->var; }
    Variable & operator()(const CompilerPtr & o) const { return o->var; }
    Variable & operator()(const CustomTargetPtr & o) const { return o->var; }
    Variable & operator()(const DependencyPtr & o) const { return o->var; }
    Variable & operator()(const DictPtr & o) const { return o->var; }
    Variable & operator()(const DisablerPtr & o) const { return o->var; }
    Variable & operator()(const ExecutablePtr & o) const { return o->var; }
    Variable & operator()(const FilePtr & o) const { return o->var; }
    Variable & operator()(const FunctionCallPtr & o) const { return o->var; }
    Variable & operator()(const IdentifierPtr & o) const { return o->var; }
    Variable & operator()(const IncludeDirectoriesPtr & o) const { return o->var; }
    Variable & operator()(const JumpPtr & o) const { return o->var; }
    Variable & operator()(const MessagePtr & o) const { return o->var; }
    Variable & operator()(const NumberPtr & o) const { return o->var; }
    Variable & operator()(const PhiPtr & o) const { return o->var; }
    Variable & operator()(const ProgramPtr & o) const { return o->var; }
    Variable & operator()(const StaticLibraryPtr & o) const { return o->var; }
    Variable & operator()(const StringPtr & o) const { return o->var; }
    Variable & operator()(const TestPtr & o) const { return o->var; }
};

struct VariableSetter {

    const Variable var;

    void operator()(AddArgumentsPtr & o) const { o->var = var; }
    void operator()(ArrayPtr & o) const { o->var = var; }
    void operator()(BooleanPtr & o) const { o->var = var; }
    void operator()(BranchPtr & o) const { o->var = var; }
    void operator()(CompilerPtr & o) const { o->var = var; }
    void operator()(CustomTargetPtr & o) const { o->var = var; }
    void operator()(DependencyPtr & o) const { o->var = var; }
    void operator()(DictPtr & o) const { o->var = var; }
    void operator()(DisablerPtr & o) const { o->var = var; }
    void operator()(ExecutablePtr & o) const { o->var = var; }
    void operator()(FilePtr & o) const { o->var = var; }
    void operator()(FunctionCallPtr & o) const { o->var = var; }
    void operator()(IdentifierPtr & o) const { o->var = var; }
    void operator()(IncludeDirectoriesPtr & o) const { o->var = var; }
    void operator()(JumpPtr & o) const { o->var = var; }
    void operator()(MessagePtr & o) const { o->var = var; }
    void operator()(NumberPtr & o) const { o->var = var; }
    void operator()(PhiPtr & o) const { o->var = var; }
    void operator()(ProgramPtr & o) const { o->var = var; }
    void operator()(StaticLibraryPtr & o) const { o->var = var; }
    void operator()(StringPtr & o) const { o->var = var; }
    void operator()(TestPtr & o) const { o->var = var; }
};

void set_var(const Object & src, Object & dest);

void link_nodes(std::shared_ptr<CFGNode> predecessor, std::shared_ptr<CFGNode> successor);
void unlink_nodes(std::shared_ptr<CFGNode> predecessor, std::shared_ptr<CFGNode> successor,
                  bool recursive = true);

} // namespace MIR
