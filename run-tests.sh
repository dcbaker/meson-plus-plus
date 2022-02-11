#!/usr/bin/env bash

# Quick and dirty script to run tests over a few directories
# This is absolutely *not* a long term solution

ret=0

for x in tests/*/*/; do
    echo -n "Testing: ${x}"
    tdir=`mktemp -d`
    builddir/src/meson++ configure "${tdir}" --source-dir "${x}" 1>/dev/null
    if [ $? != 0 ]; then
        echo " - FAILED"
        rm -rf "${tdir}"
        ret=1
        continue
    fi
    ninja -C "${tdir}" 1>/dev/null
    if [ $? != 0 ]; then
        echo " - FAILED"
        rm -rf "${tdir}"
        ret=1
        continue
    fi
    echo " - SUCCESS"
done

exit $ret
