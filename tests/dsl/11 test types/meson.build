# SPDX-License-Indentifier: Apache-2.0
# Copyright © 2024 Intel Corporation

project('test options', 'cpp')

main = executable('main', 'main.cpp')

test('good', main, arguments : ['0'])
test('xfail', main, arguments : ['1'], should_fail : true)
test('skip', main, arguments : ['127'])
