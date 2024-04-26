// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <filesystem>

#include "ast_to_mir.hpp"
#include "exceptions.hpp"

namespace fs = std::filesystem;

namespace MIR {

namespace {

/// Get just the subdir, without the source_root
fs::path get_subdir(const fs::path & full_path, const State::Persistant & pstate) {
    // This works for our case, but is probably wrong in a generic sense
    auto itr = full_path.begin();
    for (const auto & seg : pstate.source_root) {
        if (*itr == seg) {
            ++itr;
        } else {
            break;
        }
    }

    fs::path fin = *itr;
    while (++itr != full_path.end()) {
        fin = fin / *itr;
    }
    return fin;
}

/**
 * Lowers AST expressions into MIR objects.
 */
struct ExpressionLowering {

    explicit ExpressionLowering(const MIR::State::Persistant & ps) : pstate{ps} {};

    const MIR::State::Persistant & pstate;

    Instruction operator()(const std::unique_ptr<Frontend::AST::String> & expr) const {
        return String{expr->value};
    };

    Instruction operator()(const std::unique_ptr<Frontend::AST::FunctionCall> & expr) const {
        // I think that a function can only be an ID, I think
        auto fname_id = std::visit(*this, expr->held);
        auto fname_ptr = std::get_if<Identifier>(fname_id.obj_ptr.get());
        if (fname_ptr == nullptr) {
            // TODO: Better error message witht the thing being called
            throw Util::Exceptions::MesonException{"Object is not callable"};
        }
        auto fname = fname_ptr->value;

        // Get the positional arguments
        std::vector<Instruction> pos{};
        for (const auto & i : expr->args->positional) {
            pos.emplace_back(std::visit(*this, i));
        }

        std::unordered_map<std::string, Instruction> kwargs{};
        for (const auto & [k, v] : expr->args->keyword) {
            auto key_obj = std::visit(*this, k);
            auto key_ptr = std::get_if<Identifier>(key_obj.obj_ptr.get());
            if (key_ptr == nullptr) {
                // TODO: better error message
                throw Util::Exceptions::MesonException{"keyword arguments must be identifiers"};
            }
            auto key = key_ptr->value;
            kwargs[key] = std::visit(*this, v);
        }

        fs::path path = get_subdir(fs::path{expr->loc.filename}, pstate);

        // We have to move positional arguments because Object isn't copy-able
        // TODO: filename is currently absolute, but we need the source dir to make it relative
        return FunctionCall{fname, std::move(pos), std::move(kwargs),
                            fs::relative(path.parent_path(), pstate.build_root)};
    };

    Instruction operator()(const std::unique_ptr<Frontend::AST::Boolean> & expr) const {
        return Boolean{expr->value};
    };

    Instruction operator()(const std::unique_ptr<Frontend::AST::Number> & expr) const {
        return Number{expr->value};
    };

    Instruction operator()(const std::unique_ptr<Frontend::AST::Identifier> & expr) const {
        return Identifier{expr->value};
    };

    Instruction operator()(const std::unique_ptr<Frontend::AST::Array> & expr) const {
        Array arr{};
        for (const auto & i : expr->elements) {
            arr.value.emplace_back(std::visit(*this, i));
        }
        return arr;
    };

    Instruction operator()(const std::unique_ptr<Frontend::AST::Dict> & expr) const {
        Dict dict{};
        for (const auto & [k, v] : expr->elements) {
            auto key_obj = std::visit(*this, k);
            auto key_ptr = std::get_if<String>(key_obj.obj_ptr.get());
            if (key_ptr == nullptr) {
                // TODO: Better error message witht the thing being called
                throw Util::Exceptions::InvalidArguments("Dictionary keys must be string");
            }

            dict.value[key_ptr->value] = std::visit(*this, v);
        }
        return std::move(dict);
    };

    Instruction operator()(const std::unique_ptr<Frontend::AST::GetAttribute> & expr) const {
        auto holding_obj = std::visit(*this, expr->holder);

        // Meson only allows methods in objects, so we can enforce that this is a function
        auto method = std::visit(*this, expr->held);
        auto func = std::get<MIR::FunctionCall>(*method.obj_ptr);
        func.holder = holding_obj;

        return std::move(func);
    };

    // XXX: all of thse are lies to get things compiling
    Instruction operator()(const std::unique_ptr<Frontend::AST::AdditiveExpression> & expr) const {
        return String{"placeholder: add"};
    };

    Instruction
    operator()(const std::unique_ptr<Frontend::AST::MultiplicativeExpression> & expr) const {
        return String{"placeholder: mul"};
    };

    Instruction operator()(const std::unique_ptr<Frontend::AST::UnaryExpression> & expr) const {
        std::string name;
        switch (expr->op) {
            case Frontend::AST::UnaryOp::NOT:
                name = "unary_not";
                break;
            case Frontend::AST::UnaryOp::NEG:
                name = "unary_neg";
                break;
            default:
                throw std::exception{}; // Should be unreachable
        }

        fs::path path = get_subdir(fs::path{expr->loc.filename}, pstate);
        std::vector<Instruction> pos{};
        pos.emplace_back(std::visit(*this, expr->rhs));

        // We have to move positional arguments because Object isn't copy-able
        // TODO: filename is currently absolute, but we need the source dir to make it relative
        return FunctionCall{name, std::move(pos),
                            fs::relative(path.parent_path(), pstate.build_root)};
    };

    Instruction operator()(const std::unique_ptr<Frontend::AST::Subscript> & expr) const {
        return String{"placeholder: subscript"};
    };

    Instruction operator()(const std::unique_ptr<Frontend::AST::Relational> & expr) const {
        std::vector<Instruction> pos{};
        pos.emplace_back(std::visit(*this, expr->lhs));
        pos.emplace_back(std::visit(*this, expr->rhs));

        std::string func_name;
        switch (expr->op) {
            case Frontend::AST::RelationalOp::EQ:
                func_name = "rel_eq";
                break;
            case Frontend::AST::RelationalOp::NE:
                func_name = "rel_ne";
                break;
            case Frontend::AST::RelationalOp::GT:
                func_name = "rel_gt";
                break;
            case Frontend::AST::RelationalOp::GE:
                func_name = "rel_ge";
                break;
            case Frontend::AST::RelationalOp::LT:
                func_name = "rel_lt";
                break;
            case Frontend::AST::RelationalOp::LE:
                func_name = "rel_le";
                break;
            case Frontend::AST::RelationalOp::AND:
                func_name = "logic_and";
                break;
            case Frontend::AST::RelationalOp::OR:
                func_name = "logic_or";
                break;
            case Frontend::AST::RelationalOp::IN:
                func_name = "contains";
                break;
            case Frontend::AST::RelationalOp::NOT_IN:
                func_name = "not_contains";
                break;
        }
        fs::path path = get_subdir(fs::path{expr->loc.filename}, pstate);

        return FunctionCall{func_name, std::move(pos),
                            fs::relative(path.parent_path(), pstate.build_root)};
    };

    Instruction operator()(const std::unique_ptr<Frontend::AST::Ternary> & expr) const {
        return String{"placeholder: tern"};
    };
};

/**
 * Lowers AST statements into MIR objects.
 */
struct StatementLowering {

    explicit StatementLowering(const MIR::State::Persistant & ps) : pstate{ps} {};

    const MIR::State::Persistant & pstate;

    BasicBlock * operator()(BasicBlock * list,
                            const std::unique_ptr<Frontend::AST::Statement> & stmt) const {
        assert(std::holds_alternative<std::monostate>(list->next));
        const ExpressionLowering l{pstate};
        list->instructions.emplace_back(std::visit(l, stmt->expr));
        assert(std::holds_alternative<std::monostate>(list->next));
        return list;
    };

    BasicBlock * operator()(BasicBlock * list,
                            const std::unique_ptr<Frontend::AST::IfStatement> & stmt) const {
        assert(list != nullptr);
        const ExpressionLowering l{pstate};

        // This is the block that all exists from the conditional web will flow
        // back into if they don't exit. I think this is safe even for cases where
        // The blocks don't really rejoin, as this will just be empty and that's fine.
        //
        // This has the added bonus of giving us a place to put phi nodes?
        auto next_block = std::make_shared<BasicBlock>();

        // The last block that was encountered, we need this to add the next block to it.
        BasicBlock * last_block;

        // Get the value of the coindition itself (`if <condition>\n`)
        assert(std::holds_alternative<std::monostate>(list->next));
        list->next = std::make_unique<Condition>(std::visit(l, stmt->ifblock.condition));

        auto * cur = std::get<std::unique_ptr<Condition>>(list->next).get();

        last_block = cur->if_true.get();
        last_block->parents.emplace(list);

        // Walk over the statements, adding them to the if_true branch.
        for (const auto & i : stmt->ifblock.block->statements) {
            last_block = std::visit(
                [&](const auto & a) {
                    assert(cur != nullptr);
                    return this->operator()(last_block, a);
                },
                i);
        }

        // We shouldn't have a condition here, this is where we wnat to put our next target
        assert(std::holds_alternative<std::monostate>(last_block->next));
        last_block->next = next_block;
        next_block->parents.emplace(last_block);

        // for each elif branch create a new condition in the `else` of the
        // Condition, then assign the condition to the `if_true`. Then go down
        // the `else` of that new block for the next `elif`
        if (!stmt->efblock.empty()) {
            for (const auto & el : stmt->efblock) {
                cur->if_false = std::make_shared<BasicBlock>(
                    std::make_unique<Condition>(std::visit(l, el.condition)));
                cur->if_false->parents.emplace(list);
                cur = std::get<std::unique_ptr<Condition>>(cur->if_false->next).get();
                last_block = cur->if_true.get();

                for (const auto & i : el.block->statements) {
                    last_block = std::visit(
                        [&](const auto & a) { return this->operator()(last_block, a); }, i);
                }

                assert(!std::holds_alternative<std::unique_ptr<Condition>>(last_block->next));
                last_block->next = next_block;
                next_block->parents.emplace(last_block);
            }
        }

        // Finally, handle an else block.
        if (stmt->eblock.block != nullptr) {
            assert(cur->if_false == nullptr);
            cur->if_false = std::make_shared<BasicBlock>();
            last_block = cur->if_false.get();
            last_block->parents.emplace(list);
            for (const auto & i : stmt->eblock.block->statements) {
                last_block =
                    std::visit([&](const auto & a) { return this->operator()(last_block, a); }, i);
            }
            assert(!std::holds_alternative<std::unique_ptr<Condition>>(last_block->next));
            last_block->next = next_block;
            next_block->parents.emplace(last_block);
        } else {
            // If we don't have an else, create a false one by putting hte next
            // block in it. this means taht if we don't go down any of the
            // branches that we proceed on correctly
            assert(cur->if_false == nullptr);
            cur->if_false = next_block;
            next_block->parents.emplace(list);
        }

        assert(std::holds_alternative<std::monostate>(next_block->next));
        // Return the raw pointer, which is fine because we're not giving the
        // caller ownership of the pointer, the other basic blocks are the owners.
        return next_block.get();
    };

    BasicBlock * operator()(BasicBlock * list,
                            const std::unique_ptr<Frontend::AST::Assignment> & stmt) const {
        assert(std::holds_alternative<std::monostate>(list->next));
        const ExpressionLowering l{pstate};
        auto target = std::visit(l, stmt->lhs);
        auto value = std::visit(l, stmt->rhs);

        // XXX: need to handle mutative assignments
        assert(stmt->op == Frontend::AST::AssignOp::EQUAL);

        // XXX: need to handle other things that can be assigned to, like subscript
        auto name_ptr = std::get_if<Identifier>(target.obj_ptr.get());
        if (name_ptr == nullptr) {
            throw Util::Exceptions::MesonException{
                "This might be a bug, or might be an incomplete implementation"};
        }
        value.var.name = name_ptr->value;

        list->instructions.emplace_back(std::move(value));
        assert(std::holds_alternative<std::monostate>(list->next));
        return list;
    };

    // XXX: None of this is actually implemented
    BasicBlock * operator()(BasicBlock * list,
                            const std::unique_ptr<Frontend::AST::ForeachStatement> & stmt) const {
        assert(std::holds_alternative<std::monostate>(list->next));
        return list;
    };
    BasicBlock * operator()(BasicBlock * list,
                            const std::unique_ptr<Frontend::AST::Break> & stmt) const {
        assert(std::holds_alternative<std::monostate>(list->next));
        return list;
    };
    BasicBlock * operator()(BasicBlock * list,
                            const std::unique_ptr<Frontend::AST::Continue> & stmt) const {
        assert(std::holds_alternative<std::monostate>(list->next));
        return list;
    };
};

} // namespace

/**
 * Lower AST representation into MIR.
 */
BasicBlock lower_ast(const std::unique_ptr<Frontend::AST::CodeBlock> & block,
                     const MIR::State::Persistant & pstate) {
    BasicBlock bl{};
    BasicBlock * current_block = &bl;
    const StatementLowering lower{pstate};
    for (const auto & i : block->statements) {
        current_block = std::visit([&](const auto & a) { return lower(current_block, a); }, i);
    }

    return bl;
}

} // namespace MIR
