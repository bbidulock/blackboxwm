#!/bin/bash

rm -f cscope.*
./autogen.sh
./configure.sh
make clean
make cscope
cscope -b
make clean all README
