#!/bin/sh

libtoolize -c
aclocal -I /usr/local/share/aclocal
autoheader
automake --foreign -a -c
autoconf

