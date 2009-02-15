#!/bin/sh -e
# Run this to generate all the initial makefiles, etc.

gnome-doc-prepare -f
autoreconf -i -f
intltoolize -f
./configure "$@"
