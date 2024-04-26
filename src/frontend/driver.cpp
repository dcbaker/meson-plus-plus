// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <fstream>
#include <iostream>

#include "driver.hpp"
#include "node.hpp"
#include "node_visitors.hpp"
#include "parser.yy.hpp"
#include "scanner.hpp"

namespace Frontend {

std::unique_ptr<AST::CodeBlock> Driver::parse(const std::string & s) {
    name = s;

    std::ifstream stream{s, std::ios_base::in | std::ios_base::binary};

    return parse(stream);
};

std::unique_ptr<AST::CodeBlock> Driver::parse(std::istream & iss) {
    auto block = std::make_unique<Frontend::AST::CodeBlock>();
    auto scanner = std::make_unique<Frontend::Scanner>(&iss, name);
    auto parser = std::make_unique<Frontend::Parser>(*scanner, block);

    int res = parser->parse();
    if (res != 0) {
        throw std::exception{};
    }

    std::vector<AST::StatementV> new_stmts{};

    // Walk over all of the statements, replacing any subdir() calls with new
    AST::SubdirVisitor sv{};
    for (auto && stmt : block->statements) {
        auto res = std::visit(sv, stmt);

        // If we have a value that means that a `subdir()` call was
        // encounted, we then wnat to add the staements from that call into
        // our new statements instead of the current `subdir()` call.
        // Otherwise just move the statement.
        if (res.has_value()) {
            auto & v = res.value();
            std::move(v->statements.begin(), v->statements.end(), std::back_inserter(new_stmts));
        } else {
            new_stmts.emplace_back(std::move(stmt));
        }
    }

    block->statements = std::move(new_stmts);

    return block;
};

} // namespace Frontend
