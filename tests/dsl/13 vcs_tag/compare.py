#!/usr/bin/env python
# SPDX-License-Identifier: Apache-2.0
# Copyright © 2024 Intel Corporation

from __future__ import annotations
import argparse
import difflib
import filecmp
import sys
import typing as T

if T.TYPE_CHECKING:
    class Arguments(T.Protocol):
        result: str
        expected: str


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('result')
    parser.add_argument('expected')
    args: Arguments = parser.parse_args()

    if filecmp.cmp(args.result, args.expected):
        return 0

    with open(args.result, 'r') as fr, open(args.expected, 'r') as fe:
        for line in difflib.ndiff(fe.readlines(), fr.readlines()):
            print(line, file=sys.stderr, end='')

    return 1


if __name__ == "__main__":
    sys.exit(main())
