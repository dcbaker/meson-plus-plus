// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#pragma once

#include <string>

namespace HIR::Machines {

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

/**
 * Detect the build machine.
 *
 * Most of this is statically detected at compile time, using macros.
 */
Info detect_build();

} // namespace HIR::Machines
