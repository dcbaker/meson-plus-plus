from __future__ import annotations
import argparse
import textwrap
import typing

if typing.TYPE_CHECKING:
    from typing_extensions import Protocol

    class Arguments(Protocol):
        header: str
        code: str


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('code')
    parser.add_argument('header')
    args: Arguments = parser.parse_args()

    with open(args.code, 'w') as f:
        f.write(textwrap.dedent(f'''\
            #include "{args.header}"

            int func() {{
                return 0;
            }}
            '''))

    with open(args.header, 'w') as f:
        f.write(textwrap.dedent('''\
            #pragma once

            int func();
            '''))


if __name__ == "__main__":
    main()
