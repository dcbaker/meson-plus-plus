// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include "exceptions.hpp"
#include "passes.hpp"
#include "private.hpp"

namespace MIR::Passes {

bool delete_unreachable(BasicBlock & block) {
    // If we see an Message object that is an error, that block will not return,
    // break it's next connection
    for (auto itr = block.instructions.begin(); itr != block.instructions.end(); ++itr) {
        if (std::holds_alternative<std::unique_ptr<Message>>(*itr)) {
            const auto & m = *std::get<std::unique_ptr<Message>>(*itr);
            if (m.level == MessageLevel::ERROR) {
                // Delete any children point to this block
                if (std::holds_alternative<std::shared_ptr<BasicBlock>>(block.next)) {
                    auto & b = *std::get<std::shared_ptr<BasicBlock>>(block.next);
                    b.parents.erase(&block);
                } else if (std::holds_alternative<std::unique_ptr<Condition>>(block.next)) {
                    const auto & con = *std::get<std::unique_ptr<Condition>>(block.next);
                    for (const auto & c : {con.if_true, con.if_false}) {
                        c->parents.erase(&block);
                    }
                }
                // Set next to monostate, there is nothing after this
                block.next = std::monostate{};

                // Delete all instructions after this message, they don't actually exists
                // This may delete additional errors, but we can't be sure
                // they're not spurious or caused by the first error
                block.instructions.erase(++itr, block.instructions.end());
                return true;
            }
        }
    }

    return false;
}

} // namespace MIR::Passes
