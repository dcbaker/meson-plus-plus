#!/usr/bin/env python
# SPDX-License-Identifier: Apache-2.0
# Copyright © 2024 Intel Corporation

from __future__ import annotations
import argparse
import difflib
import os
import subprocess
import sys
import typing as T

if T.TYPE_CHECKING:
    class Arguments(T.Protocol):
        result: str
        template: str


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument('result')
    parser.add_argument('template')
    args: Arguments = parser.parse_args()

    with open(args.result, 'r') as f:
        got = f.read()

    with open(args.template, 'r') as f:
        tmpl = f.read()

    # TODO: Cheating a bit here because Meson++ doesn't have the meson object
    # implemented yet.
    source_dir = os.path.dirname(args.template)

    v = subprocess.run(
        ["git", "-C", source_dir, "describe", "--dirty=+", "--always"],
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        universal_newlines=True,
    )
    if v.returncode != 0:
        print(v.stderr, file=sys.stderr)
        return 2
    ever = v.stdout.strip()
    expected = tmpl.replace('@VCS_TAG@', ever)

    if got == expected:
        return 0

    for line in difflib.ndiff(got.splitlines(), expected.splitlines()):
        print(line, file=sys.stderr)

    return 1


if __name__ == "__main__":
    sys.exit(main())
