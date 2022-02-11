#!/usr/bin/env bash
set -euxo pipefail

MESONPP=${1}
TESTDIR=${2}
BUILDDIR=`mktemp -d`

function cleanup() {
    rm -rf "${BUILDDIR}"
}
trap cleanup exit

${MESONPP} configure "${BUILDDIR}" -s "${TESTDIR}"
ninja -C "${BUILDDIR}"

exit 0
