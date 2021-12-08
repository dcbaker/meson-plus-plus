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

    explicit operator bool() const;

    std::string name;

    /// The version as used by value numbering, 0 means unset
    uint version;
};

class Executable {
  public:
    Executable(const Objects::Executable & exe_) : value{exe_} {};

    const Objects::Executable value;

    Variable var;
};

class StaticLibrary {
  public:
    StaticLibrary(const Objects::StaticLibrary & exe_) : value{exe_} {};

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

    uint32_t left;
    uint32_t right;

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
    std::variant<std::unique_ptr<FunctionCall>, std::unique_ptr<String>, std::unique_ptr<Boolean>,
                 std::unique_ptr<Number>, std::unique_ptr<Identifier>, std::unique_ptr<Array>,
                 std::unique_ptr<Dict>, std::unique_ptr<Compiler>, std::unique_ptr<File>,
                 std::unique_ptr<Executable>, std::unique_ptr<StaticLibrary>, std::unique_ptr<Phi>>;

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
    Identifier(const std::string & s) : value{s}, var{} {};

    const std::string value;
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

/**
 * Holds a list of instructions, and optionally a condition or next block
 */
class BasicBlock {
  public:
    BasicBlock();
    BasicBlock(std::unique_ptr<Condition> &&);

    const uint32_t index;

    /// The instructions in this block
    std::list<Object> instructions;

    /// Either nothing, a pointer to another BasicBlock, or a pointer to a Condition
    NextType next;

    /// All potential parents of this block
    std::set<BasicBlock *> parents;
};

} // namespace MIR
