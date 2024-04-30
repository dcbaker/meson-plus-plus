// SPDX-License-Indentifier: Apache-2.0
// Copyright © 2024 Intel Corporation

#include <cstdint>

#include "backend.hpp"
#include "exceptions.hpp"
#include "utils.hpp"

namespace Backends::Common {

namespace {

const int64_t SERALIZE_VERSION = 0;

}

void Test::serialize(std::ostream & stream) const {
    // Write out the test arguments in a serialized form
    stream << "BEGIN_TEST\n";
    stream << "  name:" << name << '\n';
    stream << "  exe:" << (exe.has_parent_path() ? std::string{exe} : ("./" + std::string{exe}))
           << '\n';
    stream << "  xfail:" << int{should_fail} << '\n';
    stream << "END_TEST\n";
}

std::vector<Test> deserialize_tests(std::istream & in) {
    std::vector<Test> out{};
    Test test;
    {
        std::string line;
        std::getline(in, line, '\n');
        std::vector<std::string> split = Util::split(line, ":");
        if (split.size() != 2) {
            throw Util::Exceptions::MesonException("Malformed test serialization: " + line);
        }
        const std::string k = split[0];
        const std::string v = split[1];

        if (k != "SERIAL_VERSION") {
            throw Util::Exceptions::MesonException(
                "Malformed test serialization, first line is not a version: " + line);
        }
        if (std::stoll(v) != SERALIZE_VERSION) {
            throw Util::Exceptions::MesonException(
                "Test serialization for a different version of Meson++");
        }
    }

    for (std::string line; std::getline(in, line, '\n');) {
        if (line == "BEGIN_TEST") {
            test = Test{};
        } else if (line == "END_TEST") {
            out.push_back(test);
        } else {
            size_t first = line.find_first_not_of(' ');
            std::vector<std::string> split = Util::split(line.substr(first), ":");
            if (split.size() != 2) {
                throw Util::Exceptions::MesonException("Malformed test serialization: " + line);
            }

            const std::string & k = split[0];
            const std::string & v = split[1];

            if (k == "name") {
                test.name = v;
            } else if (k == "exe") {
                test.exe = v;
            } else if (k == "xfail") {
                test.should_fail = v == "1";
            } else {
                throw std::runtime_error{"not yet implemented"};
            }
        }
    }

    return out;
}

void serialize_tests(const std::vector<Test> & tests, const fs::path & p) {
    std::ofstream out{};
    out.open(p);

    out << "SERIAL_VERSION:" << SERALIZE_VERSION << '\n';
    for (auto && test : tests) {
        test.serialize(out);
    }

    out.flush();
    out.close();
}

std::vector<Test> load_tests(const fs::path & p) {

    std::ifstream in{};
    in.open(p);
    return deserialize_tests(in);
}

} // namespace Backends::Common
