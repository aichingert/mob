@echo off

pushd C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build
CALL vcvarsall.bat x64
popd

cl.exe mob.c

