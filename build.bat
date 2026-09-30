@echo off
setlocal enabledelayedexpansion

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

set "CMAKE=C:\Users\yalsh\AppData\Local\CMake\cmake-3.30.1-windows-x86_64\bin\cmake.exe"
set "SRC=%~dp0"
set "SRC=%SRC:~0,-1%"
set "BLD=%SRC%\build"

echo.
echo === Configuring CMake (NMake Makefiles) ===
"%CMAKE%" -S "%SRC%" -B "%BLD%" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Debug
if errorlevel 1 (
    echo CMake configure FAILED
    exit /b 1
)

echo.
echo === Building ===
"%CMAKE%" --build "%BLD%"
if errorlevel 1 (
    echo Build FAILED
    exit /b 1
)

echo.
echo === Build Complete! ===
echo Executable: %BLD%\VoxelEngine.exe

endlocal