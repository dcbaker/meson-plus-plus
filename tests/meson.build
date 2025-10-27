# SPDX-License-Identifier: Apache-2.0
# Copyright © 2022-2024 Intel Corporation

prog_runner = find_program('runner.sh', dirs : meson.current_source_dir())

test_cases = {
  'dsl' : [
    '01 subdir',
    '02 trivial',
    '03 static library',
    '04 link static and exe',
    '05 include_directories',
    '06 messages',
    '07 find_program',
    '08 custom_target',
    '09 string methods',
    '10 dependency methods',
    '11 test types',
    '12 add arguments',
    '13 vcs_tag',
    '14 vcs_tag git',
  ],
}

foreach k, test_dirs : test_cases
  foreach t : test_dirs
    test(t, prog_runner, args : [mesonpp, meson.current_source_dir() / k / t], suite : k)
  endforeach
endforeach

