// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

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

#include "objects.hpp"
#include "toolchains/toolchain.hpp"

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
    Variable() : name{}, version{0} {};
    Variable(const std::string & n) : name{n}, version{0} {};
    Variable(const std::string & n, const uint32_t & v) : name{n}, version{v} {};
    Variable(const Variable & v) : name{v.name}, version{v.version} {};

    std::string name;

    /// The version as used by value numbering, 0 means unset
    uint32_t version;

    explicit operator bool() const;
    bool operator<(const Variable &) const;
    bool operator==(const Variable &) const;
};

class Executable {
  public:
    Executable(const Objects::Executable & exe_) : value{exe_}, var{} {};
    Executable(const Objects::Executable & exe_, const Variable & v) : value{exe_}, var{v} {};

    const Objects::Executable value;

    Variable var;
};

class StaticLibrary {
  public:
    StaticLibrary(const Objects::StaticLibrary & exe_) : value{exe_}, var{} {};
    StaticLibrary(const Objects::StaticLibrary & exe_, const Variable & v) : value{exe_}, var{v} {};

    const Objects::StaticLibrary value;

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
    Phi() : left{}, right{} {};
    Phi(const uint32_t & l, const uint32_t & r, const Variable & v) : left{l}, right{r}, var{v} {};

    uint32_t left;
    uint32_t right;

    bool operator==(const Phi & other) const;
    bool operator<(const Phi & other) const;

    Variable var;
};

class IncludeDirectories {
  public:
    IncludeDirectories() : value{}, var{} {};
    IncludeDirectories(const Objects::IncludeDirectories & inc) : value{inc}, var{} {};

    const Objects::IncludeDirectories value;

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
    Message(const MessageLevel & l, const std::string & m) : level{l}, message{m} {};

    /// What level or kind of message this is
    const MessageLevel level;

    /// The message itself
    const std::string message;

    Variable var;
};

/*
 * Thse objects "Wrap" a lower level object, and provide interfaces for user
 * defined data. Their main job is to take the user data, validate it, and call
 * into the lower level objects
 */
class Array;
class Boolean;
class Dict;
class FunctionCall;
class Identifier;
class Number;
class String;
class Compiler;
class File;

using Object =
    std::variant<std::shared_ptr<FunctionCall>, std::shared_ptr<String>, std::shared_ptr<Boolean>,
                 std::shared_ptr<Number>, std::unique_ptr<Identifier>, std::shared_ptr<Array>,
                 std::shared_ptr<Dict>, std::shared_ptr<Compiler>, std::shared_ptr<File>,
                 std::shared_ptr<Executable>, std::shared_ptr<StaticLibrary>, std::unique_ptr<Phi>,
                 std::shared_ptr<IncludeDirectories>, std::unique_ptr<Message>>;

/**
 * Holds a toolchain
 *
 * Called compiler as that's what it is in the Meson DSL
 */
class Compiler {
  public:
    Compiler(const std::shared_ptr<MIR::Toolchain::Toolchain> & tc) : toolchain{tc} {};

    const std::shared_ptr<MIR::Toolchain::Toolchain> toolchain;

    const Object get_id(const std::vector<Object> &,
                        const std::unordered_map<std::string, Object> &) const;

    Variable var;
};

/**
 * Holds a File, which is a smart object point to a source
 *
 */
class File {
  public:
    File(const Objects::File & f) : file{f} {};

    const Objects::File file;

    Variable var;
};

// Can be a method via an optional paramter maybe?
/// A function call object
class FunctionCall {
  public:
    FunctionCall(const std::string & _name, std::vector<Object> && _pos,
                 std::unordered_map<std::string, Object> && _kw, const std::filesystem::path & _sd)
        : name{_name}, pos_args{std::move(_pos)}, kw_args{std::move(_kw)}, holder{std::nullopt},
          source_dir{_sd}, var{} {};

    const std::string name;

    /// Ordered container of positional argument objects
    std::vector<Object> pos_args;

    /// Unordered container mapping keyword arguments to their values
    std::unordered_map<std::string, Object> kw_args;

    /// name of object holding this function, if it's a method.
    std::optional<std::string> holder;

    /**
     * The directory this was called form.
     *
     * For functions that care (such as file(), and most targets()) this is
     * required to accurately map sources between the source and build dirs.
     */
    const std::filesystem::path source_dir;

    Variable var;
};

class String {
  public:
    String(const std::string & f) : value{f}, var{} {};

    const std::string value;
    Variable var;
};

class Boolean {
  public:
    Boolean(const bool & f) : value{f}, var{} {};

    const bool value;
    Variable var;
};

class Number {
  public:
    Number(const int64_t & f) : value{f}, var{} {};

    const int64_t value;
    Variable var;
};

class Identifier {
  public:
    Identifier(const std::string & s) : value{s}, version{}, var{} {};
    Identifier(const std::string & s, const uint32_t & ver, Variable && v)
        : value{s}, version{ver}, var{std::move(v)} {};

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

    Variable var;
};

class Array {
  public:
    Array() : value{}, var{} {};
    Array(std::vector<Object> && a) : value{std::move(a)} {};

    std::vector<Object> value;
    Variable var;
};

class Dict {
  public:
    Dict() : value{}, var{} {};

    // TODO: the key is allowed to be a string or an expression that evaluates
    // to a string, we need to enforce that somewhere.
    std::unordered_map<std::string, Object> value;
    Variable var;
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
    Condition(Object && o);
    Condition(Object && o, std::shared_ptr<BasicBlock> s);

    /// An object that is the condition
    Object condition;

    /// The block to go to if the condition is true
    std::shared_ptr<BasicBlock> if_true;

    /// The block to go to if the condition is false
    std::shared_ptr<BasicBlock> if_false;
};

using NextType =
    std::variant<std::monostate, std::unique_ptr<Condition>, std::shared_ptr<BasicBlock>>;

class BasicBlock;

struct BBComparitor {
    bool operator()(const BasicBlock * lhs, const BasicBlock * rhs) const;
};

/**
 * Holds a list of instructions, and optionally a condition or next block
 */
class BasicBlock {
  public:
    BasicBlock();
    BasicBlock(std::unique_ptr<Condition> &&);

    /// The instructions in this block
    std::list<Object> instructions;

    /// Either nothing, a pointer to another BasicBlock, or a pointer to a Condition
    NextType next;

    /// All potential parents of this block
    std::set<BasicBlock *, BBComparitor> parents;

    const uint32_t index;

    bool operator<(const BasicBlock &) const;
};

} // namespace MIR
