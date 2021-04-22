// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

#include <cassert>
#include <cstdint>

#ifdef HAVE_ENDIAN_H
#include <endian.h>
#endif

#include "machines.hpp"

namespace Meson::Machines {

namespace {

/**
 * Detect the endianness for the build machine
 */
Endian detect_endian() {
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
Kernel detect_kernel() {
#if __linux__
    return Kernel::LINUX;
#else
#error("This Kernel is currently unsupported")
#endif
}

// This must be a string unfortunately, as the user is free to se this to a
// value we don't determine in their machine files.
std::string detect_cpu_family() {
#ifdef __x86_64__
    return "x86_64";
#elif __i386__
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
    }
    assert(false);
}

} // namespace Meson::Machines
