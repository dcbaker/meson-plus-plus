#!/usr/bin/env bash
set -euxo pipefail

SOURCEDIR="${1}"

rm -rf "${SOURCEDIR}/.git"
