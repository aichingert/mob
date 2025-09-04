@echo off

cl ./src/mob.c ./src/arena.c ./src/parser.c ./src/tokenize.c ./src/writer.c ./src/compile.c /link /out:prog.exe
