#!/bin/bash

# Compile and install on Linux

# Special case for my own convenience:
if [ "${SLACKWARE:-no}" = "yes" ]; then
  VERSION=${VERSION:-2.2svn}
  ARCH=${ARCH:-i486}
  BUILD=${BUILD:-1}
  TAG=${TAG:-bkw}
  TMP=${TMP:-/tmp}
  DOCDIR=/usr/doc/doom64ex-2.2
  MANDIR=/usr/man
fi

DESTDIR=${1:-/}
PREFIX=${PREFIX:-/usr}
BINDIR=${BINDIR:-$PREFIX/bin}
SHAREDIR=${SHAREDIR:-$PREFIX/share}
MANDIR=${MANDIR:-$PREFIX/share/man}
DOCDIR=${DOCDIR:-$PREFIX/share/doc/doom64ex}
ICONDIR=${ICONDIR:-$SHAREDIR/pixmaps}
DESKTOPDIR=${DESKTOPDIR:-$SHAREDIR/applications}
CONTENTDIR=${CONTENTDIR:-$SHAREDIR/games/doom64/Content}

set -e

if [ ! -e src ]; then
  cd ..
fi

mkdir -p \
  $DESTDIR/$CONTENTDIR \
  $DESTDIR/$BINDIR \
  $DESTDIR/$MANDIR/man6 \
  $DESTDIR/$DOCDIR \
  $DESTDIR/$ICONDIR \
  $DESTDIR/$DESKTOPDIR

rm -rf build

( mkdir build
  cd build
  cmake ../src/kex/
  make
  strip doom64ex
  mv doom64ex $DESTDIR/$BINDIR
)

rm -rf build

( mkdir build
  cd build
  cmake ../src/wadgen
  make
  strip WadGen
  mv WadGen $DESTDIR/$BINDIR/doom64ex-wadgen
)

rm -rf build

cp -r src/wadgen/Content/* $DESTDIR/$CONTENTDIR
cp linux/doom64ex.desktop $DESTDIR/$DESKTOPDIR
cp linux/doom64ex.png $DESTDIR/$ICONDIR
gzip -9c linux/doom64ex.6 > $DESTDIR/$MANDIR/man6/doom64ex.6.gz
cp -r src/kex/DOCS/* $DESTDIR/$DOCDIR
find $DESTDIR -name .svn | xargs rm -rf

if [ "${SLACKWARE:-no}" = "yes" ]; then
  mkdir -p $DESTDIR/install
  cat linux/slack-desc > $DESTDIR/install/slack-desc
  cat linux/doinst.sh > $DESTDIR/install/doinst.sh
fi

if [ "${SLACKWARE:-no}" = "yes" -a "$(id -u)" = "0" ]; then
  cd $DESTDIR
  makepkg -l y -c n $TMP/doom64ex-$VERSION-$ARCH-$BUILD$TAG.${PKGTYPE:-tgz}
fi
