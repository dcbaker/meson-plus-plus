// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Intel Corporation

/* Common helpers for toolchains
 */

#pragma once

namespace HIR::Toolchain {

/**
 * If the tool (compiler, linker, archiver) support response files, and which
 * dialect they use.
 */
enum class RSPFileSupport {
    NONE,
    MSVC,
    GCC,
};

} // namespace HIR::Toolchain
