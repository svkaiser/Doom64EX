#!/bin/sh
set -e

arch=$(uname -m)
case "$arch" in
    x86_64|amd64)
        bindir=linux64
        ;;
    x86|i*86)
        bindir=linux32
        ;;
esac

if [ -z $bindir ]; then
    echo "Unknown architecture: $arch"
    exit
fi

export LD_LIBRARY_PATH=$bindir
"${bindir}/doom64ex" "$@"
