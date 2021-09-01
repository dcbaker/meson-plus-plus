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
#include <string>
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
                 std::unique_ptr<Executable>>;

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
 * A sort of phi-like thing that holds a condition and two branches
 *
 * We use shared ptrs as it's possible for the
 */
class Condition {
  public:
    Condition(Object && o)
        : condition{std::move(o)}, if_true{std::make_unique<BasicBlock>()},
          // We could save a bit of memory here by not initializing if_false, but
          // that means more manual tracking for a tiny savings…
          if_false{std::make_unique<BasicBlock>()} {};

    /// An object that is the condition
    Object condition;

    /// The branch to take if the condition is true
    std::shared_ptr<BasicBlock> if_true;

    /// The branch to take if the condition is false
    std::shared_ptr<BasicBlock> if_false;
};

/**
 * Holds a list of instructions, and optionally a condition or next point
 *
 * Jump is used when the list unconditionally jumps to another basic block, and
 * thus condition should be nullopt. This is meant for cases such as:
 *
 *       / 2 \
 * 0 - 1 - 3 - 4
 *
 *  In this case both 2 and 3 unconditionally continue to 4, but we don't want
 *  two copies of 4, just one, they both will "jump" to 4.
 *
 * On the other hand 1 does not make any unconditional jumps, and it uses the
 * condition node to set a condition as to whether it goes to 2 or 3.
 *
 * TOOD: maybe it's better to use a variant/union?
 *
 */
class BasicBlock {
  public:
    BasicBlock() : instructions{}, condition{std::nullopt}, next{nullptr} {};

    /// The instructions in this block
    std::list<Object> instructions;

    /// A phi-like condition that may come at the end of the block
    std::optional<Condition> condition;

    /// The next basic block to go to.
    std::shared_ptr<BasicBlock> next;
};

} // namespace MIR
