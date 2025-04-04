// SPDX-License-Identifier: Apache-2.0
// Copyright © 2024-2025 Intel Corporation

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

    Object operator()(const std::unique_ptr<Frontend::AST::String> & expr) const {
        return std::make_shared<String>(expr->value);
    };

    Object operator()(const std::unique_ptr<Frontend::AST::FunctionCall> & expr) const {
        // I think that a function can only be an ID, I think
        auto fname_id = std::visit(*this, expr->held);
        IdentifierPtr fname_ptr;
        try {
            fname_ptr = std::get<IdentifierPtr>(fname_id);
        } catch (std::bad_variant_access &) {
            // TODO: Better error message witht the thing being called
            throw Util::Exceptions::MesonException{"Object is not callable"};
        }
        const std::string & fname = fname_ptr->value;

        // Get the positional arguments
        std::vector<Object> pos{};
        for (const auto & i : expr->args->positional) {
            pos.emplace_back(std::visit(*this, i));
        }

        std::unordered_map<std::string, Object> kwargs{};
        for (const auto & [k, v] : expr->args->keyword) {
            auto key_obj = std::visit(*this, k);
            try {
                auto key_ptr = std::get<IdentifierPtr>(key_obj);
                kwargs.emplace(key_ptr->value, std::visit(*this, v));
            } catch (std::bad_variant_access &) {
                // TODO: Better error message witht the thing being called
                throw Util::Exceptions::MesonException{"keyword arguments must be identifiers"};
            }
        }

        const fs::path subdir = get_subdir(fs::path{expr->loc.filename}, pstate);

        // We have to move positional arguments because Object isn't copy-able
        // TODO: filename is currently absolute, but we need the source dir to make it relative
        return std::make_shared<FunctionCall>(fname, std::move(pos), std::move(kwargs),
                                              std::move(subdir));
    };

    Object operator()(const std::unique_ptr<Frontend::AST::Boolean> & expr) const {
        return std::make_shared<Boolean>(expr->value);
    };

    Object operator()(const std::unique_ptr<Frontend::AST::Number> & expr) const {
        return std::make_shared<Number>(expr->value);
    };

    Object operator()(const std::unique_ptr<Frontend::AST::Identifier> & expr) const {
        if (expr->value == "meson") {
            return std::make_shared<Meson>();
        }
        return std::make_shared<Identifier>(expr->value);
    };

    Object operator()(const std::unique_ptr<Frontend::AST::Array> & expr) const {
        auto arr = std::make_shared<Array>();
        for (const auto & i : expr->elements) {
            arr->value.emplace_back(std::visit(*this, i));
        }
        return arr;
    };

    Object operator()(const std::unique_ptr<Frontend::AST::Dict> & expr) const {
        auto dict = std::make_shared<Dict>();
        for (const auto & [k, v] : expr->elements) {
            auto key_obj = std::visit(*this, k);
            try {
                auto key_ptr = std::get<StringPtr>(key_obj);
                dict->value.emplace(key_ptr->value, std::visit(*this, v));
            } catch (std::bad_variant_access &) {
                // TODO: Better error message witht the thing being called
                throw Util::Exceptions::MesonException{"Dictionary keys must be strings"};
            }
        }
        return std::move(dict);
    };

    Object operator()(const std::unique_ptr<Frontend::AST::GetAttribute> & expr) const {
        MIR::Object holding_obj = std::visit(*this, expr->holder);

        // Meson only allows methods in objects, so we can enforce that this is a function
        MIR::Object method = std::visit(*this, expr->held);
        auto func = std::get<MIR::FunctionCallPtr>(method);
        func->holder = holding_obj;

        return std::move(func);
    };

    // XXX: all of thse are lies to get things compiling
    Object operator()(const std::unique_ptr<Frontend::AST::AdditiveExpression> & expr) const {
        return std::make_shared<String>("placeholder: add");
    };

    Object operator()(const std::unique_ptr<Frontend::AST::MultiplicativeExpression> & expr) const {
        return std::make_shared<String>("placeholder: mul");
    };

    Object operator()(const std::unique_ptr<Frontend::AST::UnaryExpression> & expr) const {
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
        std::vector<Object> pos{};
        pos.emplace_back(std::visit(*this, expr->rhs));

        // We have to move positional arguments because Object isn't copy-able
        // TODO: filename is currently absolute, but we need the source dir to make it relative
        return std::make_shared<FunctionCall>(name, std::move(pos), path);
    };

    Object operator()(const std::unique_ptr<Frontend::AST::Subscript> & expr) const {
        return std::make_shared<String>("placeholder: subscript");
    };

    Object operator()(const std::unique_ptr<Frontend::AST::Relational> & expr) const {
        std::vector<Object> pos{};
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

        return std::make_shared<FunctionCall>(func_name, std::move(pos), path);
    };

    Object operator()(const std::unique_ptr<Frontend::AST::Ternary> & expr) const {
        return std::make_shared<String>("placeholder: tern");
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
        auto branch = std::make_shared<MIR::Branch>();

        {
            auto if_node = std::make_shared<CFGNode>();
            link_nodes(head, if_node);
            branch->branches.emplace_back(std::visit(l, stmt->ifblock.condition), if_node);
            for (auto && i : stmt->ifblock.block->statements) {
                if_node =
                    std::visit([&](const auto & a) { return this->operator()(if_node, a); }, i);
            }

            // Finally, insert a jump from the last block reached by the if
            // pointing to our tail block.
            if_node->block->instructions.emplace_back(std::make_shared<Jump>(tail));
            link_nodes(if_node, tail);
        }

        if (!stmt->efblock.empty()) {
            for (const auto & el : stmt->efblock) {
                auto if_node = std::make_shared<CFGNode>();
                link_nodes(head, if_node);
                branch->branches.emplace_back(std::visit(l, el.condition), if_node);
                for (auto && i : el.block->statements) {
                    if_node =
                        std::visit([&](const auto & a) { return this->operator()(if_node, a); }, i);
                }
                // Finally, insert a jump from the last block reached by the if
                // pointing to our tail block.
                if_node->block->instructions.emplace_back(std::make_shared<Jump>(tail));
                link_nodes(if_node, tail);
            }
        }

        if (stmt->eblock.block) {
            auto if_node = std::make_shared<CFGNode>();
            link_nodes(head, if_node);
            branch->branches.emplace_back(std::make_shared<Boolean>(true), if_node);
            for (auto && i : stmt->eblock.block->statements) {
                if_node =
                    std::visit([&](const auto & a) { return this->operator()(if_node, a); }, i);
            }

            // Finally, insert a jump from the last block reached by the if
            // pointing to our tail block.
            if_node->block->instructions.emplace_back(std::make_shared<Jump>(tail));
            link_nodes(if_node, tail);
        } else {
            // This case there's an implicit else block, to jump to the tail
            branch->branches.emplace_back(std::make_shared<Boolean>(true), tail);
            link_nodes(head, tail);
        }
        head->block->instructions.insert(head->block->instructions.end(), branch);

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

        MIR::IdentifierPtr name_ptr;
        try {
            name_ptr = std::get<IdentifierPtr>(target);
        } catch (std::bad_variant_access &) {
            // TODO: Better error message with the thing being called
            throw Util::Exceptions::MesonException{
                "Expected an Identifier but got something else. This might be a bug, or might be "
                "an incomplete implementation"};
        }
        std::visit(MIR::VariableSetter{name_ptr->value}, value);

        list->block->instructions.emplace_back(value);
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
