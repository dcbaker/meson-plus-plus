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
  private:
    std::shared_ptr<CFGNode> current;
    std::deque<std::weak_ptr<CFGNode>> todo;
    std::set<uint32_t> seen;

    void add_todo(std::shared_ptr<CFGNode> b) {
        if (b && !block_worked(b->index) && all_predecessors_seen(b)) {
            todo.emplace_front(b);
        }
    }

    bool all_predecessors_seen(const std::shared_ptr<CFGNode> b) const {
        return std::all_of(
            b->predecessors.begin(), b->predecessors.end(),
            [this](const std::weak_ptr<CFGNode> p) { return this->block_worked(p.lock()->index); });
    }

    bool block_worked(uint32_t b) const { return seen.find(b) != seen.end(); }

  public:
    BlockIterator(std::shared_ptr<CFGNode> c) { add_todo(c); };

    std::shared_ptr<CFGNode> get() {
        if (current) {
            for (auto && c : current->successors) {
                add_todo(c);
            }
        }

        while (!todo.empty()) {
            current = todo.back().lock();
            todo.pop_back();
            if (current) {
                assert(!block_worked(current->index));
                seen.emplace(current->index);
                return current;
            }
        }
        return nullptr;
    }
};

} // namespace

bool instruction_walker(CFGNode & block, const std::vector<MutationCallback> & fc) {
    return instruction_walker(block, fc, {});
}

bool instruction_walker(CFGNode & block, const std::vector<ReplacementCallback> & rc) {
    return instruction_walker(block, {}, rc);
}

bool instruction_walker(CFGNode & block, const std::vector<MutationCallback> & fc,
                        const std::vector<ReplacementCallback> & rc) {
    bool progress = false;

    for (auto it = block.block->instructions.begin(); it != block.block->instructions.end(); ++it) {
        for (const auto & cb : rc) {
            auto rt = cb(*it);
            if (rt.has_value()) {
                it = block.block->instructions.erase(it);
                it = block.block->instructions.insert(it, std::move(rt.value()));
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

bool function_walker(CFGNode & block, const ReplacementCallback & cb) {
    bool progress = instruction_walker(
        block,
        {
            [&](const Instruction & obj) { return array_walker(obj, cb); }, // look into arrays
            // look into function arguments
            [&](const Instruction & obj) { return function_argument_walker(obj, cb); },
            // TODO: look into dictionary elements
        },
        {cb});

    // TODO: conditions where previously handled here
    // What to do about predicated jumps?

    return progress;
};

bool function_walker(CFGNode & block, const MutationCallback & cb) {
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

    // TODO: conditions where previously handled here
    // What to do about predicated jumps?

    return progress;
};

bool graph_walker(std::shared_ptr<CFGNode> root, const std::vector<BlockWalkerCb> & callbacks) {
    bool progress = false;

    BlockIterator iter{root};
    while (std::shared_ptr<CFGNode> current = iter.get()) {
        for (const auto & cb : callbacks) {
            progress |= cb(current);
        }
    }

    return progress;
}

} // namespace MIR::Passes
