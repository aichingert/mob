#!/bin/sh

set -e

gcc -std=c23 -ffreestanding bootstrap.c -o bootstrap

if test -n "$1" && test "$1" = "r" ; then
    ./bootstrap
fi

