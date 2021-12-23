// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

#include <deque>
#include <set>

#include "exceptions.hpp"
#include "private.hpp"

namespace MIR::Passes {

namespace {

bool replace_elements(std::vector<Object> & vec, const ReplacementCallback & cb) {
    bool progress = false;
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        auto rt = cb(*it);
        if (rt.has_value()) {
            vec[it - vec.begin()] = std::move(rt.value());
            progress |= true;
        }
    }
    return progress;
}

} // namespace

bool instruction_walker(BasicBlock * block, const std::vector<MutationCallback> & fc) {
    return instruction_walker(block, fc, {});
}

bool instruction_walker(BasicBlock * block, const std::vector<ReplacementCallback> & rc) {
    return instruction_walker(block, {}, rc);
}

bool instruction_walker(BasicBlock * block, const std::vector<MutationCallback> & fc,
                        const std::vector<ReplacementCallback> & rc) {
    bool progress = false;

    for (auto it = block->instructions.begin(); it != block->instructions.end(); ++it) {
        for (const auto & cb : rc) {
            auto rt = cb(*it);
            if (rt.has_value()) {
                it = block->instructions.erase(it);
                it = block->instructions.insert(it, std::move(rt.value()));
                progress |= true;
            }
        }
        for (const auto & cb : fc) {
            progress |= cb(*it);
        }
    }

    return progress;
};

bool array_walker(Object & obj, const MutationCallback & cb) {
    bool progress = false;

    if (!std::holds_alternative<std::shared_ptr<Array>>(obj)) {
        return progress;
    }
    auto & arr = std::get<std::shared_ptr<Array>>(obj);

    for (auto & e : arr->value) {
        if (std::holds_alternative<std::shared_ptr<Array>>(e)) {
            progress |= array_walker(e, cb);
        } else {
            progress |= cb(e);
        }
    }

    return progress;
}

bool array_walker(const Object & obj, const ReplacementCallback & cb) {
    bool progress = false;

    if (!std::holds_alternative<std::shared_ptr<Array>>(obj)) {
        return progress;
    }
    auto & arr = std::get<std::shared_ptr<Array>>(obj);

    for (auto it = arr->value.begin(); it != arr->value.end(); ++it) {
        if (std::holds_alternative<std::shared_ptr<Array>>(*it)) {
            progress |= array_walker(*it, cb);
        } else {
            auto rt = cb(*it);
            if (rt.has_value()) {
                arr->value[it - arr->value.begin()] = std::move(rt.value());
                progress |= true;
            }
        }
    }

    return progress;
}

bool function_argument_walker(const Object & obj, const ReplacementCallback & cb) {
    bool progress = false;

    if (!std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        return progress;
    }

    auto & func = std::get<std::shared_ptr<FunctionCall>>(obj);

    if (!func->pos_args.empty()) {
        progress |= replace_elements(func->pos_args, cb);
    }

    if (!func->kw_args.empty()) {
        for (auto & [n, v] : func->kw_args) {
            if (std::holds_alternative<std::shared_ptr<Array>>(v)) {
                progress |= array_walker(v, cb);
            } else if (std::optional<Object> o = cb(v); o.has_value()) {
                func->kw_args[n] = std::move(o.value());
                progress = true;
            }
        }
    }

    return progress;
}

bool function_argument_walker(Object & obj, const MutationCallback & cb) {
    bool progress = false;

    if (!std::holds_alternative<std::shared_ptr<FunctionCall>>(obj)) {
        return progress;
    }

    auto & func = std::get<std::shared_ptr<FunctionCall>>(obj);

    for (auto & e : func->pos_args) {
        progress |= cb(e);
        progress |= array_walker(e, cb);
    }

    if (!func->kw_args.empty()) {
        for (auto & [_, v] : func->kw_args) {
            progress |= cb(v);
            progress |= array_walker(v, cb);
        }
    }

    return progress;
}

bool function_walker(BasicBlock * block, const ReplacementCallback & cb) {
    bool progress = instruction_walker(
        block,
        {
            [&](Object & obj) { return array_walker(obj, cb); }, // look into arrays
            // look into function arguments
            [&](Object & obj) { return function_argument_walker(obj, cb); },
            // TODO: look into dictionary elements
        },
        {cb});

    // Check if we have a condition, and try to lower that as well.
    if (std::holds_alternative<std::unique_ptr<Condition>>(block->next)) {
        auto & con = std::get<std::unique_ptr<Condition>>(block->next);
        auto new_value = cb(con->condition);
        if (new_value.has_value()) {
            con->condition = std::move(new_value.value());
            progress |= true;
        }
    }

    return progress;
};

bool block_walker(BasicBlock * root, const std::vector<BlockWalkerCb> & callbacks) {
    std::deque<BasicBlock *> todo{};
    BasicBlock * current = root;
    bool progress = false;

    while (true) {
        // It's possible that we need to walk over the same block twice in a
        // loop because the block has been mutated such that running the same
        // test on it will result in a different result.
        for (const auto & cb : callbacks) {
            progress |= cb(current);
        }

        if (std::holds_alternative<std::unique_ptr<Condition>>(current->next)) {
            const auto & con = std::get<std::unique_ptr<Condition>>(current->next);
            if (con->if_false != nullptr) {
                todo.push_front(con->if_false.get());
            }
            if (con->if_true != nullptr) {
                todo.push_front(con->if_true.get());
            }
        } else if (std::holds_alternative<std::shared_ptr<BasicBlock>>(current->next)) {
            auto bb = std::get<std::shared_ptr<BasicBlock>>(current->next);
            if (bb != nullptr) {
                todo.push_front(bb.get());
            }
        }

        if (todo.empty()) {
            break;
        }

        // Grab the next block, if we haven't visited all of it's parents, then
        // skip it and come back after we've visited the remaining parent(s).
        // It's safe to just drop it off the todo stack, as it will be added
        // back after visiting the next parent.
        current = todo.back();
        todo.pop_back();
    }

    return progress;
}

} // namespace MIR::Passes
