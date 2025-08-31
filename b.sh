#!/bin/sh

clang -DGOC_SELF_BUILD=0 src/*c -o goc && ./goc
