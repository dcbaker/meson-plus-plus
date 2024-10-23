# SPDX-License-Identifier: Apache-2.0
# Copyright © 2024 Intel Corporation

project(
  'vcs_tag test',
  version: '1.0.0',
)

python = find_program('python')
tester = files('compare.py')

# TODO: this will not work correctly whenever we implement support for VCs.
x = vcs_tag(input: 'basic.in', output: 'basic.out')
test('basic', python, args: [tester, x, files('basic.expected')])

x = vcs_tag(input: 'basic.in', output: 'fallback.out', fallback : '"fallback version"')
test('fallback', python, args: [tester, x, files('fallback.expected')])
