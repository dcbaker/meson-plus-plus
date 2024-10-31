// SPDX-License-Indentifier: Apache-2.0
// Copyright © 2024 Intel Corporation

#include "tools/test.hpp"
#include "util/log.hpp"

#include <array>
#include <iostream>
#include <mutex>
#include <optional>
#include <thread>

namespace Tools {

namespace {

namespace bs = Backends::Common;
namespace fs = std::filesystem;

class Jobs {
  public:
    Jobs(std::vector<bs::Test> t) : count{t.size()}, tests{std::move(t)} {};

    void run(const fs::path & builddir) {
        while (true) {
            std::optional<bs::Test> m_test = get();
            if (!m_test) {
                return;
            }
            auto && test = m_test.value();
            std::vector<std::string> cmd{test.exe};
            cmd.insert(cmd.end(), test.arguments.begin(), test.arguments.end());
            auto && [ret, out, err] = Util::process(cmd, builddir.c_str());

            std::lock_guard l{print_lock};

            bool print_captured = false;
            std::string result;
            switch (ret) {
                case 0:
                    if (test.should_fail) {
                        results.xpass++;
                        result = Util::Log::red("XPASS");
                        print_captured = true;
                    } else {
                        results.success++;
                        result = Util::Log::green("OK");
                    }
                    break;
                case 127:
                    results.skipped++;
                    result = Util::Log::yellow("SKIP");
                    break;
                default:
                    if (test.should_fail) {
                        result = Util::Log::green("XFAIL");
                        results.xfail++;
                    } else {
                        result = Util::Log::red("FAIL");
                        results.failures++;
                        print_captured = true;
                    }
            }

            if (print_captured) {
                std::cout << out << std::endl;
                std::cerr << err << std::endl;
            }

            // TODO: need to calculate lengths
            // TODO: actually print the project name
            std::cout << " " << finished++ << "/" << count << " ";
            std::cout << "project name"
                      << " / " << test.name << "    " << result << "   "
                      << "<time>" << std::endl;
        }
    }

    void report() const {
        std::cout << std::endl;
        std::cout << "Ok:              " << results.success << std::endl;
        std::cout << "Fail:            " << results.failures << std::endl;
        std::cout << "Skipped:         " << results.skipped << std::endl;
        std::cout << "Expected Fail:   " << results.xfail << std::endl;
        std::cout << "Unexpected Pass: " << results.xpass << std::endl;
    }

    int status() const { return (results.failures > 0 || results.xpass > 0) ? 1 : 0; }

  private:
    size_t count;
    std::vector<bs::Test> tests;
    size_t finished{0};

    struct Results {
        size_t success{0};
        size_t failures{0};
        size_t skipped{0};
        size_t xfail{0};
        size_t xpass{0};
    } results;

    std::mutex job_lock{};
    std::mutex print_lock{};

    std::optional<bs::Test> get() {
        std::lock_guard l{job_lock};
        if (tests.empty()) {
            return std::nullopt;
        }
        bs::Test t = tests.back();
        tests.pop_back();
        return t;
    }
};

} // namespace

int run_tests(const std::vector<bs::Test> & tests, const fs::path & builddir) {
    std::array<std::thread, 8> threads;
    Jobs jobs{tests};

    for (unsigned i = 0; i < 8; ++i) {
        threads[i] = std::thread([&] { jobs.run(builddir); });
    }

    for (auto && t : threads) {
        t.join();
    }

    jobs.report();

    return jobs.status();
}

} // namespace Tools
