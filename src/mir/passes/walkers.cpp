// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2025 Intel Corporation

#include "exceptions.hpp"
#include "private.hpp"

#include <algorithm>
#include <deque>
#include <set>
#include <utility>

namespace MIR::Passes {

namespace {

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

bool mutation_visitor(Object & it, MutationCallback cb) {
    bool progress = false;

    if (std::holds_alternative<MIR::ArrayPtr>(it)) {
        auto a = std::get<MIR::ArrayPtr>(it);
        for (auto & a : a->value) {
            progress |= mutation_visitor(a, cb);
        }
    } else if (std::holds_alternative<MIR::DictPtr>(it)) {
        auto d = std::get<MIR::DictPtr>(it);
        for (auto & [_, v] : d->value) {
            progress |= mutation_visitor(v, cb);
        }
    } else if (std::holds_alternative<MIR::FunctionCallPtr>(it)) {
        auto f = std::get<MIR::FunctionCallPtr>(it);
        for (auto & p : f->pos_args) {
            progress |= mutation_visitor(p, cb);
        }
        for (auto & [_, v] : f->kw_args) {
            progress |= mutation_visitor(v, cb);
        }
        if (f->holder) {
            progress |= mutation_visitor(f->holder.value(), cb);
        }
    } else if (std::holds_alternative<MIR::JumpPtr>(it)) {
        auto j = std::get<MIR::JumpPtr>(it);
        if (j->predicate) {
            progress |= mutation_visitor(*j->predicate, cb);
        }
    } else if (std::holds_alternative<MIR::BranchPtr>(it)) {
        auto b = std::get<MIR::BranchPtr>(it);
        for (auto & [i, _] : b->branches) {
            progress |= mutation_visitor(i, cb);
        }
    }
    progress |= cb(it);

    return progress;
}

std::tuple<bool, std::optional<Object>> replacement_visitor(const Object & it,
                                                            const ReplacementCallback & cb);

bool replace_elements(std::vector<Object> & vec, const ReplacementCallback & cb) {
    bool progress = false;
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        auto && [rp, rt] = replacement_visitor(*it, cb);
        progress |= rp;
        if (rt.has_value()) {
            vec[it - vec.begin()] = rt.value();
            progress |= true;
        }
    }
    return progress;
}

bool replace_elements(std::unordered_map<std::string, Object> & map,
                      const ReplacementCallback & cb) {
    bool progress = false;
    for (auto it = map.begin(); it != map.end(); ++it) {
        auto && [rp, rt] = replacement_visitor(it->second, cb);
        progress |= rp;
        if (rt.has_value()) {
            map[it->first] = rt.value();
            progress |= true;
        }
    }
    return progress;
}

std::tuple<bool, std::optional<Object>> replacement_visitor(const Object & it,
                                                            const ReplacementCallback & cb) {
    bool progress{false};

    if (std::holds_alternative<MIR::ArrayPtr>(it)) {
        auto a = std::get<MIR::ArrayPtr>(it);
        progress |= replace_elements(a->value, cb);
    } else if (std::holds_alternative<MIR::DictPtr>(it)) {
        // TODO: keys
        auto d = std::get<MIR::DictPtr>(it);
        progress |= replace_elements(d->value, cb);
    } else if (std::holds_alternative<MIR::FunctionCallPtr>(it)) {
        auto f = std::get<MIR::FunctionCallPtr>(it);
        progress |= replace_elements(f->pos_args, cb);
        progress |= replace_elements(f->kw_args, cb);
        if (f->holder) {
            auto && [rp, rt] = replacement_visitor(f->holder.value(), cb);
            progress |= rp;
            if (rt) {
                f->holder = rt.value();
            }
        }
    } else if (std::holds_alternative<MIR::JumpPtr>(it)) {
        auto j = std::get<MIR::JumpPtr>(it);
        if (j->predicate) {
            auto && [rp, rt] = replacement_visitor(*j->predicate, cb);
            progress |= rp;
            if (rt) {
                j->predicate = rt.value();
                progress |= true;
            }
        }
    } else if (std::holds_alternative<MIR::BranchPtr>(it)) {
        auto b = std::get<MIR::BranchPtr>(it);
        for (auto it2 = b->branches.begin(); it2 != b->branches.end(); ++it2) {
            auto && [rp, rt] = replacement_visitor(std::get<0>(*it2), cb);
            progress |= rp;
            if (rt) {
                b->branches[it2 - b->branches.begin()] =
                    std::make_tuple(rt.value(), std::get<1>(*it2));
                progress |= true;
            }
        }
    }
    auto && rt = cb(it);
    progress |= rt.has_value();
    return std::make_tuple(progress, rt);
}

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
            auto && [rp, rt] = replacement_visitor(*it, cb);
            progress |= rp;
            if (rt) {
                it = block.block->instructions.erase(it);
                it = block.block->instructions.insert(it, rt.value());
                progress |= true;
            }
        }
        for (const auto & cb : fc) {
            progress |= mutation_visitor(*it, cb);
        }
    }

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
