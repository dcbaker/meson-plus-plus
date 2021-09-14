// SPDX-license-identifier: Apache-2.0
// Copyright © 2021 Dylan Baker

/**
 * Main Meson++ entrypoint
 */

#include "options.hpp"

int main(int argc, char * argv[]) {
    const auto opts = Options::parse_opts(argc, argv);

    return 0;
}
