# SPDX-License-Indentifier: Apache-2.0
# Copyright © 2024 Intel Corporation

project('test options', 'cpp')

main = executable('main', 'main.cpp')

test('good', main, args : ['0'])
test('xfail', main, args : ['1'], should_fail : true)
test('skip', main, args : ['127'])
