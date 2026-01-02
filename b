#!/bin/sh

set -e

clang -std=c23 -ffreestanding bootstrap.c -o bootstrap

if test -n "$1" && test "$1" = "r" ; then
    ./bootstrap             \
        "std/types.c"       \
        "std/mem.c"         \
        "std/io.c"          \
        "std/sort.c"        \
        "std/math.c"        \
        "std/file.c"        \
        "std/arena.c"       \
        "std/string.c"      \
        "std/collection.c"  \
        "std/unix_os.c"     \
        "std/unix_sys.c"    \
        "std/unix_socket.c" \
        "std/unix_window.c" \
                            \
        "mob.c"                 
fi

