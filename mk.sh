#!/bin/sh

[ -d /usr/local/share/aclocal ] \
    && ${ACLOCAL-aclocal} -I /usr/local/share/aclocal
    || ${ACLOCAL-aclocal}
${LIBTOOLIZE-libtoolize} -c
${AUTOHEADER-autoheader}
${AUTOMAKE-automake} --foreign -a -c
${AUTOCONF-autoconf}

rm -rf autom4te.cache
