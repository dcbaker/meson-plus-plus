#!/usr/bin/env bash

# Quick and dirty script to run tests over a few directories
# This is absolutely *not* a long term solution

for x in tests/*/*/; do
    echo -n "Testing: ${x}"
    tdir=`mktemp -d`
    build/src/meson++ configure "${tdir}" --source-dir "${x}" 1>/dev/null
    if [ $? != 0 ]; then
        echo " - FAILED"
        rm -rf "${tdir}"
        exit 1
    fi
    ninja -C "${tdir}" 1>/dev/null
    if [ $? != 0 ]; then
        echo " - FAILED"
        rm -rf "${tdir}"
        exit 1
    fi
    echo " - SUCCESS"
done

exit 0
