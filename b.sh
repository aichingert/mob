#!/bin/sh

clang -DMOB_SELF_BUILD=0 src/*c -o mob && ./mob
