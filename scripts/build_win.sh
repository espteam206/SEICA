#!/bin/sh

set -xe

cmake -B build_win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw64.cmake
cmake --build build_win
export WINEPATH="/usr/x86_64-w64-mingw32/sys-root/mingw/bin/"
export WINEDEBUG="fixme-all"
