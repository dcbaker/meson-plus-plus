// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <cassert>
#include <cstdint>

#ifdef HAVE_ENDIAN_H
#include <endian.h>
#endif

#include "machines.hpp"

namespace MIR::Machines {

namespace {

/**
 * These are all const evaluated at build time.
 *
 * This means there is 0 overhead for any of this, the compiler is able it
 * inline all of this at build time. Since none of this can be changed (ie, the
 * build machine is the build machine), that's perfect.
 */

/**
 * Detect the endianness for the build machine
 */
constexpr Endian detect_endian() {
#ifdef HAVE_ENDIAN_H
#if __BYTE_ORDER == __LITTLE_ENDIAN
    return Endian::LITTLE;
#elif __BYTE_ORDER == __BIG_ENDIAN
    return Endian::BIG;
#else
#error("WHAT?")
#endif
#else
#error("Unable to detect endiandess for this platform!")
#endif
}

/**
 * Detect the Operating System kernel
 */
constexpr Kernel detect_kernel() {
#if defined(__linux__)
    return Kernel::LINUX;
#else
#error("This Kernel is currently unsupported")
#endif
}

// This must be a string unfortunately, as the user is free to se this to a
// value we don't determine in their machine files.
constexpr const char * detect_cpu_family() {
#if defined(__x86_64__)
    return "x86_64";
#elif defined(__i386__)
    return "x86";
#else
#error("this cpu family is not supported")
#endif
}

} // namespace

Info detect_build() {
    return Info{Machine::BUILD, detect_kernel(), detect_endian(), detect_cpu_family()};
}

const std::string Info::system() const {
    switch (kernel) {
        case Kernel::LINUX:
            return "linux";
        default:
            assert(false);
    }
}

} // namespace MIR::Machines
