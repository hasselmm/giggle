#!/bin/sh -e
# Run this to generate all the initial makefiles, etc.

autoreconf -i -f
intltoolize -f
./configure "$@"
