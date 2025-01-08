// SPDX-License-Identifier: Apache-2.0
// Copyright © 2024-2025 Intel Corporation

#include "argument_extractors.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

bool custom_target_program_replacement(Object & obj) {
    if (!std::holds_alternative<MIR::FunctionCallPtr>(obj)) {
        return false;
    }

    auto & fc = std::get<MIR::FunctionCallPtr>(obj);
    if (fc->name != "custom_target") {
        return false;
    }

    const auto & cmd_obj_v =
        extract_keyword_argument_v<MIR::ArrayPtr, MIR::StringPtr>(
            fc->kw_args, "command");
    // TODO: should this be an error?
    if (std::holds_alternative<std::monostate>(cmd_obj_v)) {
        return false;
    }

    if (std::holds_alternative<MIR::ArrayPtr>(cmd_obj_v)) {
        auto commands = std::get<MIR::ArrayPtr>(cmd_obj_v)->value;
        // TODO: should this be an error?
        if (commands.empty()) {
            return false;
        }
        Object & cmd = commands.at(0);
        if (std::holds_alternative<MIR::StringPtr>(cmd)) {
            commands[0] = std::make_shared<MIR::FunctionCall>(
                "find_program", std::vector<Object>{std::move(cmd)}, fc->source_dir);
            fc->kw_args["command"] = std::make_shared<Array>(std::move(commands));
        }
    } else if (std::holds_alternative<MIR::StringPtr>(cmd_obj_v)) {
        auto && c = std::get<MIR::StringPtr>(cmd_obj_v);
        auto fp = std::make_shared<MIR::FunctionCall>(
            "find_program", std::vector<Object>{std::move(c)},
            fc->source_dir);
        fc->kw_args["command"] = std::make_shared<Array>(std::vector<Object>{std::move(fp)});
    } else {
        return false;
    }
    return true;
}

} // namespace MIR::Passes
