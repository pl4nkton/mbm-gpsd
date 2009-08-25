#!/bin/sh
# Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.
REQUIRED_AUTOMAKE_VERSION=1.7
PKG_NAME=mbm-gpsd



(test -f $srcdir/configure.in \
  && test -f $srcdir/src/mbm_gpsd.c) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level $PKG_NAME directory"
    exit 1
}

(cd $srcdir;
    autoreconf --install --symlink &&
    autoreconf &&
    glib-gettextize --copy --force &&
    intltoolize --copy --force &&
    ./configure --enable-maintainer-mode $@;
	make -f Makeversion    
)

