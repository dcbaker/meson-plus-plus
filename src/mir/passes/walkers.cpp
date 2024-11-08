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

bool mutation_visitor(Instruction & it, MutationCallback cb) {
    bool progress = false;

    if (auto * a = std::get_if<MIR::Array>(it.obj_ptr.get())) {
        for (auto & a : a->value) {
            progress |= mutation_visitor(a, cb);
        }
    } else if (auto * d = std::get_if<MIR::Dict>(it.obj_ptr.get())) {
        for (auto & [_, v] : d->value) {
            progress |= mutation_visitor(v, cb);
        }
    } else if (auto * f = std::get_if<MIR::FunctionCall>(it.obj_ptr.get())) {
        for (auto & p : f->pos_args) {
            progress |= mutation_visitor(p, cb);
        }
        for (auto & [_, v] : f->kw_args) {
            progress |= mutation_visitor(v, cb);
        }
        progress |= mutation_visitor(f->holder, cb);
    } else if (auto * j = std::get_if<MIR::Jump>(it.obj_ptr.get())) {
        if (j->predicate) {
            progress |= mutation_visitor(*j->predicate, cb);
        }
    } else if (auto * b = std::get_if<MIR::Branch>(it.obj_ptr.get())) {
        for (auto & [i, _] : b->branches) {
            progress |= mutation_visitor(i, cb);
        }
    }
    progress |= cb(it);

    return progress;
}

bool replacement_visitor(const Instruction & it, const ReplacementCallback & cb);

bool replace_elements(std::vector<Instruction> & vec, const ReplacementCallback & cb) {
    bool progress = false;
    for (auto it = vec.begin(); it != vec.end(); ++it) {
        progress |= replacement_visitor(*it, cb);
        auto rt = cb(*it);
        if (rt.has_value()) {
            vec[it - vec.begin()] = rt.value();
            progress |= true;
        }
    }
    return progress;
}

bool replace_elements(std::unordered_map<std::string, Instruction> & map,
                      const ReplacementCallback & cb) {
    bool progress = false;
    for (auto it = map.begin(); it != map.end(); ++it) {
        progress |= replacement_visitor(it->second, cb);
        auto rt = cb(it->second);
        if (rt.has_value()) {
            map[it->first] = rt.value();
            progress |= true;
        }
    }
    return progress;
}

bool replacement_visitor(const Instruction & it, const ReplacementCallback & cb) {
    bool progress = false;

    if (auto * a = std::get_if<MIR::Array>(it.obj_ptr.get())) {
        progress |= replace_elements(a->value, cb);
    } else if (auto * d = std::get_if<MIR::Dict>(it.obj_ptr.get())) {
        progress |= replace_elements(d->value, cb);
    } else if (auto * f = std::get_if<MIR::FunctionCall>(it.obj_ptr.get())) {
        progress |= replace_elements(f->pos_args, cb);
        progress |= replace_elements(f->kw_args, cb);
        progress |= replacement_visitor(f->holder, cb);
    } else if (auto * j = std::get_if<MIR::Jump>(it.obj_ptr.get())) {
        if (j->predicate) {
            progress |= replacement_visitor(*j->predicate, cb);
            if (auto rt = cb(*j->predicate)) {
                j->predicate = std::make_shared<Instruction>(rt.value());
                progress |= true;
            }
        }
    } else if (auto * b = std::get_if<MIR::Branch>(it.obj_ptr.get())) {
        for (auto it2 = b->branches.begin(); it2 != b->branches.end(); ++it2) {
            progress |= replacement_visitor(std::get<0>(*it2), cb);
            if (auto rt = cb(std::get<0>(*it2))) {
                b->branches[it2 - b->branches.begin()] =
                    std::make_tuple(rt.value(), std::get<1>(*it2));
                progress |= true;
            }
        }
    }
    return progress;
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
            progress |= replacement_visitor(*it, cb);
            if (auto rt = cb(*it)) {
                it = block.block->instructions.erase(it);
                it = block.block->instructions.insert(it, std::move(rt.value()));
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
