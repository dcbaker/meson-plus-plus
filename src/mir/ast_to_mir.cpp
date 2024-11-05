// SPDX-License-Identifier: Apache-2.0
// Copyright © 2024 Intel Corporation

#include <filesystem>

#include "ast_to_mir.hpp"
#include "exceptions.hpp"

namespace fs = std::filesystem;

namespace MIR {

namespace {

/// Get just the subdir, without the source_root
fs::path get_subdir(const fs::path & full_path, const State::Persistant & pstate) {
    // This works for our case, but is probably wrong in a generic sense
    return fs::relative(full_path, pstate.source_root).parent_path();
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

        const fs::path subdir = get_subdir(fs::path{expr->loc.filename}, pstate);

        // We have to move positional arguments because Object isn't copy-able
        // TODO: filename is currently absolute, but we need the source dir to make it relative
        return FunctionCall{fname, std::move(pos), std::move(kwargs), std::move(subdir)};
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
        return FunctionCall{name, std::move(pos), path};
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

        return FunctionCall{func_name, std::move(pos), path};
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

    std::shared_ptr<CFGNode>
    operator()(std::shared_ptr<CFGNode> list,
               const std::unique_ptr<Frontend::AST::Statement> & stmt) const {
        const ExpressionLowering l{pstate};
        list->block->instructions.emplace_back(std::visit(l, stmt->expr));
        return list;
    };

    std::shared_ptr<CFGNode>
    operator()(std::shared_ptr<CFGNode> head,
               const std::unique_ptr<Frontend::AST::IfStatement> & stmt) const {
        assert(head);
        const ExpressionLowering l{pstate};

        // TODO: we could optimize here by deciding if we have any elif/else
        // statements, and using a predicated jump?

        // We will create a branch node, which will be placed at the end of the
        // head node, this will, in turn, link to all of the subsequent nodes,
        // which will return to a newly created tail node.
        auto tail = std::make_shared<CFGNode>();
        MIR::Branch branch{};

        {
            auto if_node = std::make_shared<CFGNode>();
            link_nodes(head, if_node);
            branch.branches.emplace_back(std::visit(l, stmt->ifblock.condition), if_node);
            for (auto && i : stmt->ifblock.block->statements) {
                if_node =
                    std::visit([&](const auto & a) { return this->operator()(if_node, a); }, i);
            }

            // Finally, insert a jump from the last block reached by the if
            // pointing to our tail block.
            if_node->block->instructions.emplace_back(Jump(tail));
            link_nodes(if_node, tail);
        }

        if (!stmt->efblock.empty()) {
            for (const auto & el : stmt->efblock) {
                auto if_node = std::make_shared<CFGNode>();
                link_nodes(head, if_node);
                branch.branches.emplace_back(std::visit(l, el.condition), if_node);
                for (auto && i : el.block->statements) {
                    if_node =
                        std::visit([&](const auto & a) { return this->operator()(if_node, a); }, i);
                }
                // Finally, insert a jump from the last block reached by the if
                // pointing to our tail block.
                if_node->block->instructions.emplace_back(Jump(tail));
                link_nodes(if_node, tail);
            }
        }

        if (stmt->eblock.block) {
            auto if_node = std::make_shared<CFGNode>();
            link_nodes(head, if_node);
            branch.branches.emplace_back(Instruction{Boolean{true}}, if_node);
            for (auto && i : stmt->eblock.block->statements) {
                if_node =
                    std::visit([&](const auto & a) { return this->operator()(if_node, a); }, i);
            }

            // Finally, insert a jump from the last block reached by the if
            // pointing to our tail block.
            if_node->block->instructions.emplace_back(Jump(tail));
            link_nodes(if_node, tail);
        } else {
            // This case there's an implicit else block, to jump to the tail
            branch.branches.emplace_back(Instruction{Boolean{true}}, tail);
            link_nodes(head, tail);
        }
        head->block->instructions.insert(head->block->instructions.end(),
                                         Instruction{std::move(branch)});

        return tail;
    };

    std::shared_ptr<CFGNode>
    operator()(std::shared_ptr<CFGNode> list,
               const std::unique_ptr<Frontend::AST::Assignment> & stmt) const {
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

        list->block->instructions.emplace_back(std::move(value));
        return list;
    };

    // XXX: None of this is actually implemented
    std::shared_ptr<CFGNode>
    operator()(std::shared_ptr<CFGNode> list,
               const std::unique_ptr<Frontend::AST::ForeachStatement> & stmt) const {
        return list;
    };
    std::shared_ptr<CFGNode> operator()(std::shared_ptr<CFGNode> list,
                                        const std::unique_ptr<Frontend::AST::Break> & stmt) const {
        return list;
    };
    std::shared_ptr<CFGNode>
    operator()(std::shared_ptr<CFGNode> list,
               const std::unique_ptr<Frontend::AST::Continue> & stmt) const {
        return list;
    };
};

} // namespace

/**
 * Lower AST representation into MIR.
 */
CFG lower_ast(const std::unique_ptr<Frontend::AST::CodeBlock> & block,
              const MIR::State::Persistant & pstate) {
    auto root_block = std::make_shared<CFGNode>();
    auto current_block = root_block;
    const StatementLowering lower{pstate};
    for (const auto & i : block->statements) {
        current_block = std::visit([&](const auto & a) { return lower(current_block, a); }, i);
    }

    return CFG{root_block};
}

} // namespace MIR
