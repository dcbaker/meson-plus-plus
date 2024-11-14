// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

/**
 * Lowering passes for MIR
 */

#pragma once

#include "machines.hpp"
#include "mir.hpp"
#include "state/state.hpp"
#include "toolchains/toolchain.hpp"

#include <fstream>
#include <map>

namespace MIR::Passes {

/**
 * Prune dead condition branches
 *
 * Once we've been able to lower away conditions in the condition ndoes we want
 * to trim away dead branches and join the ir lists together so we end up with a
 * single flat list of Objects.
 */
bool branch_pruning(std::shared_ptr<CFGNode>);

/**
 * Join basic blocks together
 *
 * Specifically for use after branch_pruning, when we have two continguous
 * blocks with no condition to move between thme
 */
bool join_blocks(std::shared_ptr<CFGNode>);

/**
 * Lower away machine related information.
 *
 * This replaces function calls to `host_machine`, `build_machine`, and
 * `target_machine` methods with their values.
 */
std::optional<Instruction> machine_lower(const Instruction &,
                                         const MIR::Machines::PerMachine<MIR::Machines::Info> &);

/**
 * Run complier detection code and replace variables with compiler objects.
 */
std::optional<Instruction>
insert_compilers(const Instruction &,
                 const std::unordered_map<
                     MIR::Toolchain::Language,
                     MIR::Machines::PerMachine<std::shared_ptr<MIR::Toolchain::Toolchain>>> &);

/**
 * Find string arguments to custom_target's program space (intput[0]), and
 * replace it with a call to `find_program()`
 */
bool custom_target_program_replacement(Instruction &);

/**
 * Lowering for free functions
 *
 * This lowers free standing functions (those not part of an object/namespace).
 */
bool lower_free_functions(std::shared_ptr<CFGNode>, const State::Persistant &);

/**
 * Flatten array arguments to functions.
 *
 * If it makes sense, remove the array altogether and replace it with scalars.
 *
 * Meson allows for some interesting arrangements of functions, for example
 * project(['foo'], ['c'])
 * project(['foo', 'c'])
 * project(['foo'], 'c')
 * project('foo', 'c')
 * project('foo', ['c'])
 *
 * Are all semantically identical. Meson handles this with a method that
 * flattens arguments at call time. The interpreter class reduces the arguments
 * (except in a few cases), and then the interpreter method deal with an
 * idealized form of the function arguments.
 *
 * Meson++ uses this pass to flatten arguments, building an idealized set of
 * arguments for each function.
 */
std::optional<Instruction> flatten(const Instruction &);

struct GlobalValueNumbering {
    bool operator()(std::shared_ptr<CFGNode>);

  private:
    std::unordered_map<uint32_t, std::unordered_map<std::string, uint32_t>> data;
    std::unordered_map<std::string, uint32_t> gvn;
    bool number(Instruction &, const uint32_t);
    bool insert_phis(CFGNode &);
};

bool fixup_phis(std::shared_ptr<CFGNode>);

struct ConstantFolding {
    bool operator()(std::shared_ptr<CFGNode>);

  private:
    std::map<Variable, Variable> data;
    std::optional<Instruction> impl(const Instruction &);
};

/**
 * push variables out of assignments into their uses
 */
struct ConstantPropagation {
    bool operator()(std::shared_ptr<CFGNode>);

  private:
    std::map<Variable, Instruction *> data;
    bool update_data(Instruction &);
    std::optional<Instruction> get(const Identifier & id) const;
    std::optional<Instruction> impl(const Instruction & obj) const;
    bool impl(Instruction & obj) const;
};

/**
 * Do work that can be more optimally handled in threads.
 *
 * Examples of this are:
 *  - dependencies
 *  - find_programs
 *  - compiler checks
 *
 * These can be done in parallel, using the cache
 */
bool threaded_lowering(std::shared_ptr<CFGNode>, State::Persistant & pstate);

/**
 * Lower Program objects and their methods
 */
bool lower_program_objects(std::shared_ptr<CFGNode>, State::Persistant & pstate);

/// Lower string object methods
bool lower_string_objects(std::shared_ptr<CFGNode> block, State::Persistant & pstate);

/// Lower dependency object methods
bool lower_dependency_objects(std::shared_ptr<CFGNode> block, State::Persistant & pstate);

/// Delete any code that has become unreachable
bool delete_unreachable(std::shared_ptr<CFGNode> block);

/// Debugging pass that dumps a human readable text representation of the IR to
/// a file.
/// controlled by setting the MESONPP_DEBUG_PRINT_MIR environment variable
class Printer {
  public:
    Printer(uint32_t p = 0);
    ~Printer();
    bool operator()(const std::shared_ptr<CFGNode>);
    void increment();
    uint32_t pass;

  private:
    std::ofstream out{};
};

/// @brief Move AddArgument nodes to the top of the program
/// @param block The block to operate on
/// @return true if any work is done, otherwise false
bool combine_add_arguments(std::shared_ptr<CFGNode> block);

} // namespace MIR::Passes
