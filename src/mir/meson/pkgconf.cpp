// SPDX-license-identifier: Apache-2.0
// Copyright © 2022 Dylan Baker

#include "pkgconf.hpp"

#include <algorithm>

namespace MIR::Dependencies {

// XXX: need something more robust than this…
PkgConf::PkgConf() : context{pkgconfpp::Context::initialize()} {};

State::Dependency PkgConf::operator()(const std::string & name, const std::string & version) {
    State::Dependency dep{};

    const pkgconfpp::Query & query = context.query(name); // TODO: version
    if (!query.valid()) {
        return dep;
    }

    // TODO: It probably makes more sense to use the compiler classifier to handle this
    // So we either need to pass the Compilers in here, or we need to store this
    // as raw string arguments. Both have drawbacks.
    // TODO: As a result of the above, arguments get reordered.
    auto && inc = query.include_flags();
    std::transform(inc.begin(), inc.end(), std::back_inserter(dep.compile),
                   [](const std::string & s) {
                       return Arguments::Argument{s.substr(2), Arguments::Type::INCLUDE,
                                                  Arguments::IncludeType::BASE};
                   });

    // TODO: split DEFINES out
    auto && cflags = query.compile_flags();
    std::transform(cflags.begin(), cflags.end(), std::back_inserter(dep.compile),
                   [](const std::string & s) {
                       return Arguments::Argument{s, Arguments::Type::RAW};
                   });

    auto && lsearch = query.link_search_flags();
    std::transform(lsearch.begin(), lsearch.end(), std::back_inserter(dep.link),
                   [](const std::string & s) {
                       return Arguments::Argument{s.substr(2), Arguments::Type::LINK_SEARCH};
                   });

    auto && libs = query.link_flags();
    std::transform(libs.begin(), libs.end(), std::back_inserter(dep.link),
                   [](const std::string & s) {
                       return Arguments::Argument{s.substr(2), Arguments::Type::LINK};
                   });

    auto && lflags = query.link_other_flags();
    std::transform(lflags.begin(), lflags.end(), std::back_inserter(dep.link),
                   [](const std::string & s) {
                       return Arguments::Argument{s.substr(2), Arguments::Type::RAW};
                   });

    // Only set found to true if we're completely successful
    dep.found = true;
    return dep;
};

} // namespace MIR::Dependencies
