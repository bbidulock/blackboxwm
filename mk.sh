#!/bin/sh

libtoolize -c
aclocal
autoheader
automake --foreign -a -c 
autoconf

