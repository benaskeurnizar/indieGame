@echo off
REM Build script for the BSP renderer.
REM Requires the MSVC toolchain (run from a "Developer Command Prompt for VS").
REM See ../include/README.md and ../lib/README.md for what needs to be in place first.

if not exist builds mkdir builds
pushd builds

cl -Zi /RTC1 /MDd /Od /I"..\..\include" ../win32_main.cpp ../glad.c /link /LIBPATH:"..\..\lib" gdi32.lib user32.lib Opengl32.lib

popd
