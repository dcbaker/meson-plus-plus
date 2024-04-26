// SPDX-License-Identifier: Apache-2.0
// Copyright © 2021-2024 Intel Corporation

#include <array>
#include <cstring>
#include <iostream>
#include <thread>

// TODO: a windows version of this.
#include <poll.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "process.hpp"

namespace Util {

#define READ 0
#define WRITE 1

namespace {}

Result process(const std::vector<std::string> & cmd) {
    std::string out{}, err{};
    int out_pipes[2];
    int err_pipes[2];
    if (pipe(out_pipes) != 0) {
        // Do something reall
        throw std::exception{};
    }
    if (pipe(err_pipes) != 0) {
        // Do something reall
        throw std::exception{};
    }

    pid_t pid = fork();
    if (pid == 0) {
        dup2(out_pipes[WRITE], STDOUT_FILENO);
        dup2(err_pipes[WRITE], STDERR_FILENO);
        close(out_pipes[READ]);
        close(out_pipes[WRITE]);
        close(err_pipes[READ]);
        close(err_pipes[WRITE]);

        char * c_cmd[cmd.size() + 1];
        for (unsigned i = 0; i < cmd.size(); ++i) {
            c_cmd[i] = strdup(cmd[i].c_str());
        }
        c_cmd[cmd.size()] = nullptr;

        execvp(c_cmd[0], c_cmd);
        std::cerr << "Program failed to execute: " << strerror(errno) << std::endl;
        _exit(127);
    }

    close(out_pipes[WRITE]);
    close(err_pipes[WRITE]);

    std::array<char, 16384> buffer{};
    int status;
    ssize_t count = 0;

    std::array<pollfd, 2> fds;
    fds[0] = {out_pipes[READ], POLLIN, 0};
    fds[1] = {err_pipes[READ], POLLIN, 0};

    while (true) {
        int rt = poll(fds.data(), fds.size(), 5 * 1000);

        if (rt < 0) {
            std::cerr << "Error: " << strerror(errno) << std::endl;
        } else if (rt == 0) {
            kill(pid, SIGKILL);
            for (auto const & f : fds) {
                close(f.fd);
            }
            // XXX: do something less silly here.
            throw std::exception{};
        }

        if (fds[0].revents & POLLIN) {
            count = read(out_pipes[READ], buffer.data(), buffer.size());
            out.append(buffer.begin(), buffer.begin() + count);
        }
        if (fds[1].revents & POLLIN) {
            count = read(err_pipes[READ], buffer.data(), buffer.size());
            err.append(buffer.begin(), buffer.begin() + count);
        }

        if (fds[0].revents & POLLHUP and fds[1].revents & POLLHUP) {
            break;
        }
    }

    for (auto const & f : fds) {
        close(f.fd);
    }

    while (waitpid(pid, &status, 0) == -1)
        ;

    if (status > 255) {
        status %= 255;
    }

    // On Unix-like OSes return codes > 128 are traditionally used for
    // returning error codes, 128 + n, where n is the code.
    if (status > 128) {
        status -= 128;
        status *= -1;
    }

    return Result{status, out, err};
};

} // namespace Util
