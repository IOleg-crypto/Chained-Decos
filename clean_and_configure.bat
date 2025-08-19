@echo off
echo Cleaning build directories...
if exist cmake-build-debug rmdir /s /q cmake-build-debug
if exist cmake-build-Debug rmdir /s /q cmake-build-Debug
if exist cmake-build-release rmdir /s /q cmake-build-release
if exist cmake-build-Release rmdir /s /q cmake-build-Release

echo Creating build directory...
mkdir cmake-build-debug

echo Configuring project...
cmake -G "MinGW Makefiles" -B cmake-build-debug -S . -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_C_COMPILER=E:/LLVM/bin/clang.exe -DCMAKE_CXX_COMPILER=E:/LLVM/bin/clang++.exe

echo Building project...
cmake --build cmake-build-debug -- -j 8

echo Done!
pause