// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <optional>
#include <string>

namespace Meson::Machines {

/**
 * Represents the machine that is being used
 */
enum class Machine {
    BUILD,
    HOST,
    TARGET,
};

/**
 * The Operating system kernel in use.
 */
enum class Kernel {
    LINUX,
};

/**
 * Which endianness the machine is
 */
enum class Endian {
    BIG,
    LITTLE,
};

/**
 * Information about one of the three machines (host, build, target)
 *
 * This differs from the way Meson (python) works in a couple of ways.
 */
class Info {
  public:
    Info(const Machine & m, const Kernel & k, const Endian & e, const std::string & c)
        : machine{m}, kernel{k}, endian{e}, cpu_family{c}, cpu{c} {};
    Info(const Machine & m, const Kernel & k, const Endian & e, const std::string & cf, const std::string & c)
        : machine{m}, kernel{k}, endian{e}, cpu_family{cf}, cpu{c} {};
    ~Info(){};

    const std::string system() const;

    const Machine machine;
    const Kernel kernel;
    const Endian endian;
    const std::string cpu_family;
    const std::string cpu;
};

template <typename T> class PerMachine {
  public:
    PerMachine() : _build{}, _host{std::nullopt}, _target{std::nullopt} {};
    PerMachine(T & __build, T & __host, T & __target) : _build{__build}, _host{__host}, _target{__target} {};
    PerMachine(T & __build, T & __host) : _build{__build}, _host{__host}, _target{std::nullopt} {};
    PerMachine(T & __build) : _build{__build}, _host{std::nullopt}, _target{std::nullopt} {};
    PerMachine(T && __build, T && __host, T && __target)
        : _build{std::move(__build)}, _host{std::move(__host)}, _target{std::move(__target)} {};
    PerMachine(T && __build, T && __host)
        : _build{std::move(__build)}, _host{std::move(__host)}, _target{std::move(std::nullopt)} {};
    PerMachine(T && __build)
        : _build{std::move(__build)}, _host{std::move(std::nullopt)}, _target{std::move(std::nullopt)} {};
    PerMachine(PerMachine<T> && t)
        : _build{std::move(t._build)}, _target{std::move(t._target)}, _host{std::move(t._target)} {};
    ~PerMachine(){};

    PerMachine<T> & operator=(PerMachine<T> && t) {
        _build = std::move(t._build);
        _target = std::move(t._target);
        _host = std::move(t._target);
        return *this;
    }

    T & build() const { return _build; }
    T & host() const { return _host == std::nullopt ? _build : _host; }
    T & target() const {
        if (_target != std::nullopt) {
            return _target;
        } else if (_host != std::nullopt) {
            return _host;
        } else {
            return _build;
        }
    }

  private:
    T _build;
    std::optional<T> _host;
    std::optional<T> _target;
};

/**
 * Detect the build machine.
 *
 * Most of this is statically detected at compile time, using macros.
 */
Info detect_build();

} // namespace Meson::Machines
