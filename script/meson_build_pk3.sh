#!/bin/sh
set -e

if [ -z "$MESON_SOURCE_ROOT" ]
then
	echo "This script is meant to be called from meson"
	exit 1
fi

cd "${MESON_SOURCE_ROOT}/distrib/doom64ex.pk3"
zip -r "${MESON_BUILD_ROOT}/doom64ex.pk3" .
