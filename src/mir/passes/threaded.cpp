// SPDX-license-identifier: Apache-2.0
// Copyright © 2022 Dylan Baker

#include <algorithm>
#include <array>
#include <future>
#include <iostream>
#include <mutex>

#include "argument_extractors.hpp"
#include "exceptions.hpp"
#include "log.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace fs = std::filesystem;

namespace {

/**
 * Do the actual program finding
 *
 * This looks for the first program with a given name and sets the
 *
 * TODO: handle host vs build
 */
void find_program(const std::vector<std::string> & names, std::mutex & lock,
                  State::Persistant & pstate, std::set<std::string> & programs) {
    const std::string path{std::getenv("PATH")};

    for (const std::string & name : names) {
        std::string::size_type last{0}, next{0};
        // Only schedule one finder for this program
        {
            std::lock_guard l{lock};
            if (programs.count(name)) {
                continue;
            }
            programs.emplace(name);
        }

        // TODO: the path separator may not be `:`
        while ((next = path.find(':', last)) != std::string::npos) {
            fs::path substr = path.substr(last, next - last);
            last = next + 1;

            fs::path trial = substr / name;
            if (fs::exists(trial)) {
                std::lock_guard l{lock};
                auto & map = pstate.programs.build();
                for (const auto & name : names) {
                    if (map.count(name) == 0) {
                        map[name] = trial;
                    }
                }
                std::cout << "Found program \"" << name << "\" " << Util::Log::green("YES") << " ("
                          << trial << ")" << std::endl;
                return;
            }
        }
        std::lock_guard l{lock};
        std::cout << "Found program \"" << names[0] << "\": " << Util::Log::red("NO") << std::endl;
    }
}

enum class Type {
    PROGRAM,
};

using FindJob = std::tuple<Type, std::vector<std::string>>;

class FindList {
  public:
    std::vector<FindJob> jobs;
    std::mutex lock;

    FindList() = default;
    FindList(const FindList &) = delete;
    FindList & operator=(const FindList &) = delete;
};

bool search_find_program(const FunctionCall & f, State::Persistant & pstate, FindList & jobs) {
    auto names = extract_variadic_arguments<String>(f.pos_args.begin(), f.pos_args.end());

    std::vector<std::string> ret{names.size()};
    std::transform(names.begin(), names.end(), ret.begin(),
                   [](const String & s) { return s.value; });
    jobs.jobs.emplace_back(Type::PROGRAM, ret);

    return true;
}

void worker(FindList & jobs, std::mutex & state_lock, State::Persistant & pstate,
            std::set<std::string> & programs) {
    while (true) {
        Type job;
        std::vector<std::string> names;
        {
            std::lock_guard l{jobs.lock};
            if (jobs.jobs.empty()) {
                return;
            }
            std::tie(job, names) = jobs.jobs.back();
            jobs.jobs.pop_back();
        }
        switch (job) {
            case Type::PROGRAM:
                find_program(names, state_lock, pstate, programs);
        }
    }
}

/**
 * Search calls that we want to handle in a thread
 *
 * this includes:
 *  - find_program()
 *  - dependency()
 *  - compiler.* (that run the compiler)
 *  - subproject()? We would need a hueristic to make sure we don't start
 *                  subprojects we don't need, plus some logger changes.
 */
void search_for_threaded_impl(FindList & jobs, State::Persistant & pstate) {
    // TODO: should we use promises to get a result back from this?
    std::mutex state_lock{};
    std::set<std::string> programs{};

    // TODO: Don't hardocde this
    std::array<std::thread, 8> threads{};

    for (auto && t : threads) {
        t = std::thread([&] { return worker(jobs, state_lock, pstate, programs); });
    }

    for (auto & t : threads) {
        t.join();
    }
}

std::optional<Instruction> replace_find_program(const FunctionCall & f, State::Persistant & state) {
    // We know this is safe since we've already processed this call before (hopefully)
    // We only need the first name, as all of the names should be in the mapping
    auto name = extract_positional_argument<String>(f.pos_args[0]).value().value;

    fs::path exe;
    try {
        exe = state.programs.build().at(name);
    } catch (std::out_of_range &) {
        exe = "";
    }

    bool required =
        extract_keyword_argument<Boolean>(f.kw_args, "required").value_or(Boolean{true}).value;
    if (required && exe == "") {
        throw Util::Exceptions::MesonException("Could not find required program \"" + name + "\"");
    }

    return Program{name, Machines::Machine::BUILD, exe};
}

bool search_threaded(const Instruction & obj, State::Persistant & pstate, FindList & jobs) {
    if (!std::holds_alternative<FunctionCall>(*obj.obj_ptr)) {
        return false;
    }
    const auto & f = std::get<FunctionCall>(*obj.obj_ptr);

    if (!std::holds_alternative<std::monostate>(*f.holder.obj_ptr)) {
        return false;
    }
    if (!all_args_reduced(f.pos_args, f.kw_args)) {
        return false;
    }

    if (f.name == "find_program") {
        return search_find_program(f, pstate, jobs);
    }
    return false;
}

std::optional<Instruction> replace_threaded(const Instruction & obj, State::Persistant & state) {
    if (!std::holds_alternative<FunctionCall>(*obj.obj_ptr)) {
        return std::nullopt;
    }
    const auto & f = std::get<FunctionCall>(*obj.obj_ptr);

    if (!std::holds_alternative<std::monostate>(*f.holder.obj_ptr)) {
        return std::nullopt;
    }
    if (!all_args_reduced(f.pos_args, f.kw_args)) {
        return std::nullopt;
    }

    std::optional<Instruction> i{std::nullopt};
    if (f.name == "find_program") {
        i = replace_find_program(f, state);
    }

    if (i) {
        i.value().var = obj.var;
    }

    return i;
}

} // namespace

bool threaded_lowering(BasicBlock & block, State::Persistant & pstate) {
    bool progress = false;
    FindList jobs{};

    // TODO: three step process here:
    //  1. call the block walker to gather find_program, dependency, etc
    //  2. create the threads and send them to work on filling out those futures
    //  3. call the block walker again to fill in those values
    progress |= block_walker(block, {
                                        [&](BasicBlock & b) {
                                            return function_walker(b, [&](const Instruction & obj) {
                                                return search_threaded(obj, pstate, jobs);
                                            });
                                        },
                                    });
    if (progress) {
        search_for_threaded_impl(jobs, pstate);
        progress |=
            block_walker(block, {
                                    [&](BasicBlock & b) {
                                        return function_walker(b, [&](const Instruction & obj) {
                                            return replace_threaded(obj, pstate);
                                        });
                                    },
                                });
    }
    return progress;
}

} // namespace MIR::Passes
