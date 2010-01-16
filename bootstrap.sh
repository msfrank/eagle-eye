#!/bin/sh

echo "Bootstrapping eagle-eye build environment..."
set -x
aclocal-1.7
autoconf
automake-1.7 --add-missing --copy
