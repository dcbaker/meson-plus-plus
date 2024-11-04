// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

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
class Empty;
class CustomTarget;
class Dependency;
class Test;

using Object =
    std::variant<std::monostate, FunctionCall, String, Boolean, Number, Identifier, Array, Dict,
                 Compiler, File, Executable, StaticLibrary, Phi, IncludeDirectories, Message,
                 Program, Empty, CustomTarget, Dependency, Test, AddArguments>;

using Callable = std::variant<File, Executable, Program>;

/**
 * A single instruction.
 */
class Instruction {

  public:
    Instruction();
    Instruction(Instruction &&) = default;
    Instruction(const Instruction &) = default;
    Instruction(std::shared_ptr<Object>);
    Instruction(const Object & obj);
    Instruction(Object obj, const Variable & var_);
    Instruction(Boolean val);
    Instruction(Number val);
    Instruction(String val);
    Instruction(FunctionCall val);
    Instruction(Identifier val);
    Instruction(Array val);
    Instruction(Dict val);
    Instruction(File val);
    Instruction(IncludeDirectories val);
    Instruction(Message val);
    Instruction(Empty val);
    Instruction(Dependency val);
    Instruction(CustomTarget val);
    Instruction(StaticLibrary val);
    Instruction(Executable val);
    Instruction(Phi val);
    Instruction(Program val);
    Instruction(Test val);
    Instruction(AddArguments val);

    Instruction & operator=(const Instruction &) = default;

    /// The actual object in the instruction
    /// This is a ptr because of self-referncing, Objects can hold Instructions, so we can't put an
    /// Object directly into the Instruction
    std::shared_ptr<Object> obj_ptr;

    /// The place where the instruction was defined
    Variable var;

    /// Get a const reference to the held object
    const Object & object() const;

    /// Print a human readable version of this
    std::string print() const;
};

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

    // For gtest
    friend std::ostream & operator<<(std::ostream & os, const File & f);

    const std::string name;
    const fs::path subdir;
    const bool built;
    const fs::path source_root;
    const fs::path build_root;
};

class CustomTarget {
  public:
    CustomTarget(std::string n, std::vector<Instruction> i, std::vector<File> o,
                 std::vector<std::string> c, fs::path s, std::vector<File> d,
                 std::optional<std::string> df);

    const std::string name;
    const std::vector<Instruction> inputs;
    const std::vector<File> outputs;
    const std::vector<std::string> command;
    const fs::path subdir;
    std::vector<File> depends;
    std::optional<std::string> depfile;

    /// Print a human readable version of this
    std::string print() const;
};

using ArgMap = std::unordered_map<Toolchain::Language, std::vector<Arguments::Argument>>;

enum class StaticLinkMode {
    NORMAL,
    WHOLE,
};

class StaticLibrary;

using StaticLinkage = std::tuple<StaticLinkMode, const StaticLibrary>;

class Executable {
  public:
    Executable(std::string name_, std::vector<Instruction> srcs, const Machines::Machine & m,
               fs::path sdir, ArgMap args, std::vector<StaticLinkage> s_link);

    /// The name of the target
    const std::string name;

    /// The sources (as files)
    const std::vector<Instruction> sources;

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

    /// Print a human readable version of this
    std::string print() const;
};

class StaticLibrary {
  public:
    StaticLibrary(std::string name_, std::vector<Instruction> srcs, const Machines::Machine & m,
                  fs::path sdir, ArgMap args, std::vector<StaticLinkage> s_link);

    /// The name of the target
    const std::string name;

    /// The sources (as files)
    const std::vector<Instruction> sources;

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

    /// Print a human readable version of this
    std::string print() const;
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

    /// Print a human readable version of this
    std::string print() const;
};

class IncludeDirectories {
  public:
    IncludeDirectories(std::vector<std::string> d, const bool & s);

    const std::vector<std::string> directories;
    const bool is_system;

    /// Print a human readable version of this
    std::string print() const;
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

    /// Print a human readable version of this
    std::string print() const;
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
};

class Test {
  public:
    Test(std::string n, Callable exe, std::vector<std::variant<String, File>> args, bool xfail);

    const std::string name;
    const Callable executable;
    const std::vector<std::variant<String, File>> arguments;
    const bool should_fail;

    /// Print a human readable version of this
    std::string print() const;
};

class Empty {
  public:
    Empty() = default;

    /// Print a human readable version of this
    std::string print() const;
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

    Instruction get_id(const std::vector<Instruction> &,
                       const std::unordered_map<std::string, Instruction> &) const;

    /// Print a human readable version of this
    std::string print() const;
};

// Can be a method via an optional paramter maybe?
/// A function call object
class FunctionCall {
  public:
    FunctionCall(std::string _name, std::vector<Instruction> && _pos,
                 std::unordered_map<std::string, Instruction> && _kw, std::filesystem::path _sd);
    FunctionCall(std::string _name, std::vector<Instruction> && _pos, std::filesystem::path _sd);

    const std::string name;

    /// Ordered container of positional argument objects
    std::vector<Instruction> pos_args;

    /// Unordered container mapping keyword arguments to their values
    std::unordered_map<std::string, Instruction> kw_args;

    /// reference to object holding this function, it's monostate if not
    Instruction holder;

    /**
     * The directory this was called form.
     *
     * For functions that care (such as file(), and most targets()) this is
     * required to accurately map sources between the source and build dirs.
     */
    const std::filesystem::path source_dir;

    /// Print a human readable version of this
    std::string print() const;
};

class String {
  public:
    String(std::string f);

    bool operator==(const String &) const;
    bool operator!=(const String &) const;

    std::string value;

    /// Print a human readable version of this
    std::string print() const;
};

class Boolean {
  public:
    Boolean(const bool & f);

    bool operator==(const Boolean &) const;
    bool operator!=(const Boolean &) const;

    const bool value;

    /// Print a human readable version of this
    std::string print() const;
};

class Number {
  public:
    Number(const int64_t & f);

    bool operator==(const Number &) const;
    bool operator!=(const Number &) const;

    const int64_t value;

    /// Print a human readable version of this
    std::string print() const;
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
};

class Array {
  public:
    Array() = default;
    Array(std::vector<Instruction> && a);
    Array(std::vector<String> && a);

    std::vector<Instruction> value;

    /// Print a human readable version of this
    std::string print() const;
};

class Dict {
  public:
    Dict() = default;

    // TODO: the key is allowed to be a string or an expression that evaluates
    // to a string, we need to enforce that somewhere.
    std::unordered_map<std::string, Instruction> value;

    /// Print a human readable version of this
    std::string print() const;
};

class AddArguments {
  public:
    AddArguments(ArgMap &&, bool global);

    /// Print a human readable version of this
    std::string print() const;

    ArgMap arguments;
    bool is_global;
};

class BasicBlock;

/**
 * A think that creates A conditional web.
 *
 * This works such that `if_true` will always point to a Basic block, and
 * `if_false` will either point to andother Condition or nothing. This means
 * that our web will always have a form like:
 *
 *    O --\
 *  /      \
 * O   O --\\
 *  \ /     \\
 *   O   O - O
 *    \ /   /
 *     O   /
 *      \ /
 *       O
 *
 * Because the false condition will itself be a condition.
 *
 * if_false is initialized to nullptr, and one needs to check for that.
 */
class Condition {
  public:
    Condition(Instruction && o);
    Condition(Instruction && o, std::shared_ptr<BasicBlock> s);

    /// An object that is the condition
    Instruction condition;

    /// The block to go to if the condition is true
    std::shared_ptr<BasicBlock> if_true;

    /// The block to go to if the condition is false
    std::shared_ptr<BasicBlock> if_false;

    /// Print a human readable version of this
    std::string print() const;
};

using NextType =
    std::variant<std::monostate, std::unique_ptr<Condition>, std::shared_ptr<BasicBlock>>;

class BasicBlock;

struct BBComparitor {
    bool operator()(const BasicBlock * lhs, const BasicBlock * rhs) const;
    bool operator()(const std::shared_ptr<BasicBlock> & lhs,
                    const std::shared_ptr<BasicBlock> & rhs) const;
};

/**
 * Holds a list of instructions, and optionally a condition or next block
 */
class BasicBlock {
  public:
    BasicBlock();
    BasicBlock(std::unique_ptr<Condition> &&);

    /// The instructions in this block
    std::list<Instruction> instructions;

    /// Either nothing, a pointer to another BasicBlock, or a pointer to a Condition
    NextType next;

    /// All predecessors of this block
    std::set<BasicBlock *, BBComparitor> predecessors;

    /// @brief All blocks that come after this one
    std::set<std::shared_ptr<BasicBlock>, BBComparitor> successors;

    const uint32_t index;

    bool operator<(const BasicBlock &) const;

    /// Print a human readable version of this
    std::string print() const;
};

} // namespace MIR
