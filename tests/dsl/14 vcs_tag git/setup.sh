#!/usr/bin/env bash
set -euxo pipefail

SOURCEDIR="${1}"

pushd "${SOURCEDIR}"
git init .
git add .
git commit -a -m "test commit"
git tag 1
popd
