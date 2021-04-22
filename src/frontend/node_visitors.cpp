// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <filesystem>

#include "driver.hpp"
#include "exceptions.hpp"
#include "node_visitors.hpp"

namespace Frontend::AST {

std::optional<std::unique_ptr<CodeBlock>> SubdirVisitor::operator()(const std::unique_ptr<Statement> & stmt) const {
    const auto func_ptr = std::get_if<std::unique_ptr<FunctionCall>>(&stmt->expr);
    if (func_ptr == nullptr) {
        return std::nullopt;
    }

    const auto & func = *func_ptr;

    // Meson functions are not first class, so we know that if the type is not
    // an identifier it's not what we want. The other option would be a
    // `GetAttribute` (a method).
    const auto id_ptr = std::get_if<std::unique_ptr<Identifier>>(&func->id);
    if (id_ptr == nullptr) {
        return std::nullopt;
    }

    const auto & id = *id_ptr;

    if (id->value != "subdir") {
        return std::nullopt;
    }

    auto const & args = func->args->positional;

    // Since we don't have any other kind of validation, and I'm not sure if we
    // can validate this at the AST level since we don't have strong typing
    //
    // This is unrecoverable, so just throwing here should be fine
    if (args.size() != 1) {
        // TODO: use the location data.
        throw Util::Exceptions::InvalidArguments{"subdir() requires exactly one argument."};
    }

    auto const & dir = *std::get_if<std::unique_ptr<String>>(&args[0]);
    if (dir == nullptr) {
        // TODO: use the location data.
        throw Util::Exceptions::InvalidArguments{"subdir()'s first argument must be a string."};
    }

    // This assumes that the filename is foo/meson.build
    const std::filesystem::path _p{id->loc.filename};
    const std::filesystem::path p{_p.parent_path() / dir->value / "meson.build"};
    if (!std::filesystem::exists(p)) {
        // TODO: use the location data.
        throw Util::Exceptions::InvalidArguments{"Cannot open file or directory " + std::string{p} + "."};
    }

    Driver drv{};
    return drv.parse(p);
};

std::optional<std::unique_ptr<CodeBlock>> SubdirVisitor::operator()(const std::unique_ptr<IfStatement> & stmt) const {
    SubdirVisitor sv{};
    std::vector<StatementV> new_stmts{};

    // TODO: this code is basically copied out of the driver, how can we share it?
    for (unsigned i = 0; i < stmt->ifblock.block->statements.size(); ++i) {
        auto const & substmt = stmt->ifblock.block->statements[i];
        auto res = std::visit(sv, substmt);

        // If we have a value that means that a `subdir()` call was
        // encounted, we then wnat to add the staements from that call into
        // our new statements instead of the current `subdir()` call.
        // Otherwise just move the statement.
        if (res.has_value()) {
            auto & v = res.value();
            std::move(v->statements.begin(), v->statements.end(), std::back_inserter(new_stmts));
        } else {
            new_stmts.emplace_back(std::move(stmt->ifblock.block->statements[i]));
        }
    }

    std::swap(stmt->ifblock.block->statements, new_stmts);

    // XXX: this is kinda gross...
    return std::nullopt;
};

} // namespace Frontend::AST
