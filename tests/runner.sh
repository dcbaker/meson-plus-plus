#!/usr/bin/env bash
set -euxo pipefail

MESONPP="${1}"
TESTDIR="${2}"
BUILDDIR=`mktemp -d`
SETUP_SH="${TESTDIR}/setup.sh"
TEARDOWN_SH="${TESTDIR}/teardown.sh"

function cleanup() {
    rm -rf "${BUILDDIR}"
    if [ -f "${TEARDOWN_SH}" ]; then
        bash "${TEARDOWN_SH}" "${TESTDIR}"
    fi
}
trap 'cleanup' exit

if [ -f "${SETUP_SH}" ]; then
    bash "${SETUP_SH}" "${TESTDIR}"
fi

${MESONPP} configure "${BUILDDIR}" -s "${TESTDIR}"
ninja -C "${BUILDDIR}"
${MESONPP} test "${BUILDDIR}"

exit 0
