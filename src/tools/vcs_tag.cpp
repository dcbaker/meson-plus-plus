// SPDX-License-Identifier: Apache-2.0
// Copyright © 2024 Intel Corporation

#include "vcs_tag.hpp"
#include "util/process.hpp"
#include "util/utils.hpp"

#include <cassert>
#include <fstream>
#include <iostream>
#include <optional>
#include <sstream>
#include <vector>

namespace Tools {

namespace fs = std::filesystem;

namespace {

struct VCSData {
    std::vector<std::string> command;
    fs::path dep;
};

std::optional<VCSData> find_vcs(const fs::path & source_dir) {
    // TODO: HG, Subversion, bazaar
    const fs::path gitdir = source_dir / ".git";

    if (fs::is_directory(gitdir)) {
        return VCSData{
            .command = {"git", "-C", source_dir, "describe", "--dirty=+", "--always"},
            .dep = gitdir / "logs" / "HEAD", // TODO: This doesn't work for git work trees
        };
    }

    return std::nullopt;
}

std::string get_version(const std::optional<VCSData> & vcs_o, std::string_view fallback) {
    if (!vcs_o) {
        return std::string{fallback};
    }

    const VCSData & vcs = vcs_o.value();
    auto [rc, out, err] = Util::process(vcs.command);
    if (rc != 0) {
        throw std::runtime_error{"TODO: " + err};
    }
    out.pop_back();
    return out;
}

} // namespace

int generate_vcs_tag(const std::filesystem::path & infile, const std::filesystem::path & outfile,
                     std::string_view fallback, std::string_view replacement,
                     const std::filesystem::path & source_dir, const std::string & depfile) {
    // We assume that the infile exists and has been validated by the
    // transpiler, but for debug builds we can assert here.
    assert(fs::is_regular_file(infile));

    std::ifstream istream{infile};
    std::stringstream ostream{};

    const std::optional<VCSData> vcs = find_vcs(source_dir);
    const std::string version = get_version(vcs, fallback);

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

    // Always write the file, it simplifies things
    std::ofstream out_depfile{depfile};
    out_depfile << Util::makefile_quote(outfile) << ": ";
    if (vcs) {
        out_depfile << Util::makefile_quote(vcs.value().dep);
    }
    out_depfile << std::endl;

    std::ofstream out{outfile};
    out << ostream.str();
    return 0;
}

} // namespace Tools
