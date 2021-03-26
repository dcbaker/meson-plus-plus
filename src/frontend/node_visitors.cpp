// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <filesystem>

#include "node_visitors.hpp"
#include "driver.hpp"

namespace Frontend::AST {

std::optional<std::unique_ptr<CodeBlock>> SubdirVisitor::operator()(const std::unique_ptr<Statement> & stmt) const {
    const auto func_ptr = std::get_if<std::unique_ptr<FunctionCall>>(&stmt->expr);
    if (func_ptr == nullptr) { return std::nullopt; }

    const auto & func = *func_ptr;

    // Meson functions are not first class, so we know that if the type is not
    // an identifier it's not what we want. The other option would be a
    // `GetAttribute` (a method).
    const auto id_ptr = std::get_if<std::unique_ptr<Identifier>>(&func->id);
    if (id_ptr == nullptr) { return std::nullopt; }

    const auto & id = *id_ptr;

    if (id->value != "subdir") { return std::nullopt; }

    auto const & args = func->args->positional;

    // Since we don't have any other kind of validation, and I'm not sure if we
    // can validate this at the AST level since we don't have strong typing
    //
    // This is unrecoverable, so just throwing here should be fine
    if (args.size() < 1) {
        // TODO: use the location data.
        // TODO: have our own exception class, catch it at the base and print a
        // nice message
        throw std::exception{};
    }

    auto const & dir = *std::get_if<std::unique_ptr<String>>(&args[0]);
    if (dir == nullptr) {
        throw std::exception{};
    }

    // This assumes that the filename is foo/meson.build
    const std::filesystem::path p{*id->loc.begin.filename};
    if (!std::filesystem::exists(p)) {
        // TODO: something useful here
        throw std::exception{};
    }

    Driver drv{};
    return drv.parse(p.parent_path() / dir->value / "meson.build");
};

} // namespace Frontend::AST
