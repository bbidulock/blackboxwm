#!/bin/sh

${LIBTOOLIZE-libtoolize} -c
${ACLOCAL-aclocal} -I /usr/local/share/aclocal
${AUTOHEADER-autoheader}
${AUTOMAKE-automake} --foreign -a -c
${AUTOCONF-autoconf}

rm -rf autom4te.cache
