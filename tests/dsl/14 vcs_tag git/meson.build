# SPDX-License-Identifier: Apache-2.0
# Copyright © 2024 Intel Corporation

project(
  'vcs_tag (git) test',
  version: '1.0.0',
)

python = find_program('python')
tester = files('compare.py')

# TODO: this will not work correctly whenever we implement support for VCs.
x = vcs_tag(input: 'basic.in', output: 'basic.out')
test('git', python, args: [tester, x, files('basic.in')])
