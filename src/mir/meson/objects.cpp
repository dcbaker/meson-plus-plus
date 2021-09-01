// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "objects.hpp"
#include "exceptions.hpp"

namespace MIR::Objects {

const bool File::is_built() const { return built; }

const std::string File::get_name() const { return name; }

const std::filesystem::path File::relative_to_source_dir() const {
    if (built) {
        std::error_code ec{};
        auto p = std::filesystem::relative(build_root / name, source_root, ec);
        if (ec) {
            // TODO: better error handling
            throw Util::Exceptions::MesonException{"Failed to create relative path"};
        }
        return p;
    } else {
        return name;
    }
}

const std::filesystem::path File::relative_to_build_dir() const {
    if (!built) {
        std::error_code ec{};
        auto p = std::filesystem::relative(source_root / name, build_root, ec);
        if (ec) {
            // TODO: better error handling
            throw Util::Exceptions::MesonException{"Failed to create relative path"};
        }
        return p;
    } else {
        return name;
    }
}

} // namespace MIR::Objects
