// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include "exceptions.hpp"
#include "objects.hpp"

namespace MIR::Objects {

const bool File::is_built() const { return built; }

const std::string File::get_name() const { return name; }

const std::filesystem::path File::relative_to_source_dir() const {
    if (built) {
        std::error_code ec{};
        auto p = std::filesystem::relative(build_root / subdir / name, source_root / subdir, ec);
        if (ec) {
            // TODO: better error handling
            throw Util::Exceptions::MesonException{"Failed to create relative path"};
        }
        return p;
    } else {
        return subdir / name;
    }
}

const std::filesystem::path File::relative_to_build_dir() const {
    if (!built) {
        std::error_code ec{};
        auto p = std::filesystem::relative(source_root / subdir / name, build_root / subdir, ec);
        if (ec) {
            // TODO: better error handling
            throw Util::Exceptions::MesonException{"Failed to create relative path"};
        }
        return p;
    } else {
        return subdir / name;
    }
}

bool File::operator==(const File & f) const {
    return subdir / name == f.subdir / f.name && built == f.built;
}

bool File::operator!=(const File & f) const {
    return subdir / name != f.subdir / f.name || built != f.built;
}

std::ostream & operator<<(std::ostream & os, const File & f) {
    return os << (f.is_built() ? f.build_root : f.source_root) / f.subdir / f.get_name();
}

} // namespace MIR::Objects
