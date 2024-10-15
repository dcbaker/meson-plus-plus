// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include "exceptions.hpp"
#include "private.hpp"

#include <algorithm>
#include <deque>
#include <set>
#include <utility>

namespace MIR::Passes {

namespace {

bool replace_elements(std::vector<Instruction> & vec, const ReplacementCallback & cb) {
    bool progress = false;
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        auto rt = cb(*it);
        if (rt.has_value()) {
            vec[it - vec.begin()] = rt.value();
            progress |= true;
        }
    }
    return progress;
}

class BlockIterator {
  public:
    BlockIterator(BasicBlock * c) : current{c} { seen.emplace(current); };
    BasicBlock * get() { return current; }

    bool next() {
        if (std::holds_alternative<std::unique_ptr<Condition>>(current->next)) {
            const auto & con = *std::get<std::unique_ptr<Condition>>(current->next);
            add_todo(con.if_true);
            add_todo(con.if_false);
        } else if (std::holds_alternative<std::shared_ptr<BasicBlock>>(current->next)) {
            auto bb = std::get<std::shared_ptr<BasicBlock>>(current->next);
            add_todo(bb);
        }

        while (!todo.empty()) {
            current = todo.back().lock().get();
            todo.pop_back();
            if (current != nullptr) {
                assert(!block_worked(current));
                seen.emplace(current);
                return true;
            }
        }

        return false;
    }

    bool empty() const { return current == nullptr && todo.empty(); }

  private:
    BasicBlock * current;
    std::vector<std::weak_ptr<BasicBlock>> todo;
    std::set<const BasicBlock *, BBComparitor> seen;

    void add_todo(std::shared_ptr<BasicBlock> b) {
        if (b != nullptr && !block_worked(b.get()) && all_parents_seen(b.get())) {
            todo.emplace_back(b);
        }
    }

    bool all_parents_seen(const BasicBlock * b) const {
        return std::all_of(b->parents.begin(), b->parents.end(),
                           [this](const BasicBlock * p) { return this->block_worked(p); });
    }

    bool block_worked(const BasicBlock * b) const { return seen.find(b) != seen.end(); }
};

} // namespace

bool instruction_walker(BasicBlock & block, const std::vector<MutationCallback> & fc) {
    return instruction_walker(block, fc, {});
}

bool instruction_walker(BasicBlock & block, const std::vector<ReplacementCallback> & rc) {
    return instruction_walker(block, {}, rc);
}

bool instruction_walker(BasicBlock & block, const std::vector<MutationCallback> & fc,
                        const std::vector<ReplacementCallback> & rc) {
    bool progress = false;

    for (auto it = block.instructions.begin(); it != block.instructions.end(); ++it) {
        for (const auto & cb : rc) {
            auto rt = cb(*it);
            if (rt.has_value()) {
                it = block.instructions.erase(it);
                it = block.instructions.insert(it, std::move(rt.value()));
                progress |= true;
            }
        }
        for (const auto & cb : fc) {
            progress |= cb(*it);
        }
    }

    return progress;
};

bool array_walker(Instruction & obj, const MutationCallback & cb) {
    bool progress = false;

    auto arr_ptr = std::get_if<Array>(obj.obj_ptr.get());
    if (arr_ptr == nullptr) {
        return progress;
    }

    for (auto & e : arr_ptr->value) {
        if (std::holds_alternative<Array>(*e.obj_ptr)) {
            progress |= array_walker(e, cb);
        } else {
            progress |= cb(e);
        }
    }

    return progress;
}

bool array_walker(const Instruction & obj, const ReplacementCallback & cb) {
    bool progress = false;

    auto arr_ptr = std::get_if<Array>(obj.obj_ptr.get());
    if (arr_ptr == nullptr) {
        return progress;
    }

    for (auto it = arr_ptr->value.begin(); it != arr_ptr->value.end(); ++it) {
        if (std::holds_alternative<Array>(*it->obj_ptr)) {
            progress |= array_walker(*it, cb);
        } else {
            auto rt = cb(*it);
            if (rt.has_value()) {
                arr_ptr->value[it - arr_ptr->value.begin()] = rt.value();
                progress |= true;
            }
        }
    }

    return progress;
}

bool function_argument_walker(const Instruction & obj, const ReplacementCallback & cb) {
    bool progress = false;

    auto func_ptr = std::get_if<FunctionCall>(obj.obj_ptr.get());
    if (func_ptr == nullptr) {
        return progress;
    }

    if (!func_ptr->pos_args.empty()) {
        progress |= replace_elements(func_ptr->pos_args, cb);
    }

    if (!func_ptr->kw_args.empty()) {
        for (auto & [n, v] : func_ptr->kw_args) {
            if (std::holds_alternative<Array>(*v.obj_ptr)) {
                progress |= array_walker(v, cb);
            }
            // If the callback can act on arrays (like flatten can), we need to
            // call the cb on the array, and on the array elements
            if (std::optional<Instruction> o = cb(v); o.has_value()) {
                func_ptr->kw_args[n] = o.value();
                progress |= true;
            }
        }
    }

    return progress;
}

bool function_argument_walker(Instruction & obj, const MutationCallback & cb) {
    bool progress = false;

    auto func_ptr = std::get_if<FunctionCall>(obj.obj_ptr.get());
    if (func_ptr == nullptr) {
        return progress;
    }

    for (auto & e : func_ptr->pos_args) {
        progress |= cb(e);
        progress |= array_walker(e, cb);
    }

    if (!func_ptr->kw_args.empty()) {
        for (auto & [_, v] : func_ptr->kw_args) {
            progress |= cb(v);
            progress |= array_walker(v, cb);
        }
    }

    return progress;
}

bool function_walker(BasicBlock & block, const ReplacementCallback & cb) {
    bool progress = instruction_walker(
        block,
        {
            [&](const Instruction & obj) { return array_walker(obj, cb); }, // look into arrays
            // look into function arguments
            [&](const Instruction & obj) { return function_argument_walker(obj, cb); },
            // TODO: look into dictionary elements
        },
        {cb});

    // Check if we have a condition, and try to lower that as well.
    if (std::holds_alternative<std::unique_ptr<Condition>>(block.next)) {
        auto & con = std::get<std::unique_ptr<Condition>>(block.next);
        auto new_value = cb(con->condition);
        if (new_value.has_value()) {
            con->condition = new_value.value();
            progress |= true;
        }
    }

    return progress;
};

bool function_walker(BasicBlock & block, const MutationCallback & cb) {
    bool progress = instruction_walker(
        block,
        {
            [&](Instruction & obj) { return array_walker(obj, cb); }, // look into arrays
            // look into function arguments
            [&](Instruction & obj) { return function_argument_walker(obj, cb); },
            // TODO: look into dictionary elements
            cb,
        },
        {});

    // Check if we have a condition, and try to lower that as well.
    if (std::holds_alternative<std::unique_ptr<Condition>>(block.next)) {
        auto & con = std::get<std::unique_ptr<Condition>>(block.next);
        progress |= cb(con->condition);
    }

    return progress;
};

bool block_walker(BasicBlock & root, const std::vector<BlockWalkerCb> & callbacks) {
    bool progress = false;

    BlockIterator iter{&root};
    do {
        BasicBlock * current = iter.get();
        assert(current != nullptr);
        for (const auto & cb : callbacks) {
            progress |= cb(*current);
        }
    } while (iter.next());

    return progress;
}

} // namespace MIR::Passes
