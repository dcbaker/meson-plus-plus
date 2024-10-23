// SPDX-License-Identifier: Apache-2.0
// Copyright © 2024 Intel Corporation

#include "vcs_tag.hpp"
#include "util/utils.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <sstream>

namespace Tools {

namespace fs = std::filesystem;

int generate_vcs_tag(const std::filesystem::path & infile, const std::filesystem::path & outfile,
                     std::string_view version, std::string_view replacement) {
    // We assume that the infile exists and has been validated by the
    // transpiler, but for debug builds we can assert here.
    assert(fs::is_regular_file(infile));

    std::ifstream istream{infile};
    std::stringstream ostream{};

    for (std::string line; std::getline(istream, line);) {
        ostream << Util::replace(line, replacement, version) << std::endl;
    }

    istream.close();

    // If the outfile already exists, then check if the new file and the old
    // file are the same. If they are, don't write them to avoid spurious
    // rebuilds
    if (fs::exists(outfile)) {
        std::ifstream oldstream{outfile};
        std::stringstream newstream{};
        newstream << ostream.rdbuf();
        bool equal = true;
        while (true) {
            char o, n;
            newstream.get(n);
            oldstream.get(o);

            if (newstream.eof() || oldstream.eof()) {
                equal &= newstream.eof() == oldstream.eof();
                break;
            }

            if (o != n) {
                equal = false;
                break;
            }
        }
        if (equal) {
            return 0;
        }
    }

    std::ofstream out{outfile};
    out << ostream.str();
    return 0;
}

} // namespace Tools
