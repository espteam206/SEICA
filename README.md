# desktop
The desktop alternative design for concrete carbon calculation.

## compiling
Requires CMake to build. CMake will automatically fetch all the dependencies
```
git clone https://github.com/espteam206/desktop
cd desktop
cmake -B build
cmake --build build
```

### Windows compilation from Linux
1) Install [MinGW](https://www.mingw-w64.org/downloads/)
- Note: Need to install `mingw64-gcc-c++.x86_64` and `mingw64-gcc.x86_64`
2) Install [Wine](https://www.winehq.org/)
3) `x86_64-w64-mingw32-gcc`
4) `cmake -B build -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw64.cmake`
5) to let wine know where the dll files are: `export WINEPATH="/usr/x86_64-w64-mingw32/sys-root/mingw/bin/"`
