// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <cassert>
#include <optional>
#include <string>

namespace MIR::Machines {

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

std::string to_string(const Machine);
std::string to_string(const Kernel);
std::string to_string(const Endian);

/**
 * Information about one of the three machines (host, build, target)
 *
 * This differs from the way Meson (python) works in a couple of ways.
 */
class Info {
  public:
    Info(const Machine & m, const Kernel & k, const Endian & e, const std::string & c)
        : machine{m}, kernel{k}, endian{e}, cpu_family{c}, cpu{c} {};
    Info(const Machine & m, const Kernel & k, const Endian & e, std::string cf, std::string c)
        : machine{m}, kernel{k}, endian{e}, cpu_family{std::move(cf)}, cpu{std::move(c)} {};
    ~Info() = default;

    Info(const Info &) = default;

    std::string system() const;

    const Machine machine;
    const Kernel kernel;
    const Endian endian;
    const std::string cpu_family;
    const std::string cpu;
};

template <typename T> class PerMachine {
  public:
    PerMachine() : _build{}, _host{std::nullopt}, _target{std::nullopt} {};
    PerMachine(T & _b, T & _h, T & _t) : _build{_b}, _host{_h}, _target{_t} {};
    PerMachine(T & _b, T & _h) : _build{_b}, _host{_h}, _target{std::nullopt} {};
    PerMachine(T & _b) : _build{_b}, _host{std::nullopt}, _target{std::nullopt} {};
    PerMachine(T && _b, T && _h, T && _t)
        : _build{std::move(_b)}, _host{std::move(_h)}, _target{std::move(_t)} {};
    PerMachine(T && _b, T && _h)
        : _build{std::move(_b)}, _host{std::move(_h)}, _target{std::nullopt} {};
    PerMachine(T && _b) : _build{std::move(_b)}, _host{std::nullopt}, _target{std::nullopt} {};
    PerMachine(PerMachine<T> && t) noexcept
        : _build{std::move(t._build)}, _target{std::move(t._target)}, _host{
                                                                          std::move(t._target)} {};
    ~PerMachine() = default;

    PerMachine<T> & operator=(PerMachine<T> && t) noexcept {
        _build = std::move(t._build);
        _target = std::move(t._target);
        _host = std::move(t._target);
        return *this;
    }

    T build() const { return _build; }

    T & build() { return _build; }

    T host() const { return _host.value_or(build()); }

    T & host() {
        if (_host)
            return _host.value();

        return _build;
    }

    T target() const { return _target.value_or(host()); }

    T & target() {
        if (_target)
            return _target.value();

        if (_host)
            return _host.value();

        return _build;
    }

    T get(const Machine & m) const {
        switch (m) {
            case Machine::BUILD:
                return build();
            case Machine::HOST:
                return host();
            case Machine::TARGET:
                return target();
            default:
                assert(0);
        }
    }

    void set(const Machine & m, T && new_) {
        switch (m) {
            case Machine::BUILD:
                _build = std::move(new_);
                break;
            case Machine::HOST:
                _host = std::move(new_);
                break;
            case Machine::TARGET:
                _target = std::move(new_);
                break;
            default:
                assert(0);
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

} // namespace MIR::Machines
