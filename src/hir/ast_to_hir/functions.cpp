// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include <memory>
#include <numeric>
#include <variant>

#include "functions.hpp"

namespace HIR::FromAST::Functions {

namespace {
/**
 * Check that the number of arguments i
 */
void check_num_args(Frontend::AST::ExpressionList & args, unsigned min = 0, unsigned max = 0) {
    // XXX: do something better here
    if (args.size() < min) {
        throw std::exception{};
    }
    if (max > 0 && args.size() > max) {
        throw std::exception{};
    }
};

} // namespace

void project(State::Persistant & pers, State::Transitive & trans, Frontend::AST::FunctionCall & func) {
    auto & pos = func.args->positional;
    auto & kw = func.args->keyword;
    check_num_args(pos, 1);

    auto * name_ptr = std::get_if<std::unique_ptr<Frontend::AST::String>>(&pos[0]);
    if (name_ptr == nullptr) {
        // TODO: something better here
        throw std::exception{};
    }

    auto name = (*name_ptr)->value;

    std::vector<std::string> lang_strs =
        std::accumulate(pos.begin() + 1, pos.end(), std::vector<std::string>{},
                        [](std::vector<std::string> acc, Frontend::AST::ExpressionV & expr) {
                            auto * ptr = std::get_if<std::unique_ptr<Frontend::AST::String>>(&expr);
                            if (ptr == nullptr) {
                                // TODO: something better here
                                throw std::exception{};
                            }

                            acc.emplace_back((*ptr)->value);
                            return acc;
                        });

    for (const auto & l : lang_strs) {
        auto lang = Toolchain::from_string(l);
        // In the project call we always add languages for both machines.
        pers.toolchains[lang] = Machines::PerMachine{
            Toolchain::get_toolchain(lang, Machines::Machine::BUILD),
            Toolchain::get_toolchain(lang, Machines::Machine::HOST)};
    }
};

} // namespace HIR::FromAST::Functions
