// SPDX-License-Identifier: Apache-2.0
// Copyright © 2024 Intel Corporation

#include "argument_extractors.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

bool program_replacement_impl(Instruction & obj) {
    if (!std::holds_alternative<MIR::FunctionCall>(*obj.obj_ptr)) {
        return false;
    }

    auto & fc = std::get<MIR::FunctionCall>(*obj.obj_ptr);
    if (fc.name != "custom_target") {
        return false;
    }

    const auto & cmd_obj_v =
        extract_keyword_argument_v<MIR::Array, MIR::String>(fc.kw_args, "command");
    // TODO: should this be an error?
    if (std::holds_alternative<std::monostate>(cmd_obj_v)) {
        return false;
    }

    if (std::holds_alternative<MIR::Array>(cmd_obj_v)) {
        auto commands = std::get<MIR::Array>(cmd_obj_v).value;
        // TODO: should this be an error?
        if (commands.empty()) {
            return false;
        }
        const Instruction & cmd = commands.at(0);
        if (std::holds_alternative<MIR::String>(*cmd.obj_ptr)) {
            auto s = std::get<MIR::String>(*cmd.obj_ptr);
            MIR::FunctionCall fp{"find_program", {std::move(s)}, fc.source_dir};
            commands[0] = std::move(fp);
            fc.kw_args["command"] = Array{std::move(commands)};
        }
    } else if (std::holds_alternative<MIR::String>(cmd_obj_v)) {
        auto s = std::get<MIR::String>(cmd_obj_v);
        MIR::FunctionCall fp{"find_program", {std::move(s)}, fc.source_dir};
        fc.kw_args["command"] = Array{{std::move(fp)}};
    } else {
        return false;
    }
    return true;
}

} // namespace

bool custom_target_program_replacement(std::shared_ptr<CFGNode> block) {
    return instruction_walker(*block, {program_replacement_impl});
}

} // namespace MIR::Passes
