@echo off
setlocal enabledelayedexpansion

call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvarsall.bat" x64

set "CMAKE=C:\Users\yalsh\AppData\Local\CMake\cmake-3.30.1-windows-x86_64\bin\cmake.exe"
set "SRC=%~dp0"
set "SRC=%SRC:~0,-1%"
set "BLD=%SRC%\build_release"
set "DIST=%SRC%\dist"

echo.
echo === Configuring Release Build ===
"%CMAKE%" -S "%SRC%" -B "%BLD%" -G "NMake Makefiles" -DCMAKE_BUILD_TYPE=Release
if errorlevel 1 exit /b 1

echo.
echo === Building Standalone Release Binary ===
"%CMAKE%" --build "%BLD%"
if errorlevel 1 exit /b 1

echo.
echo === Packaging Standalone Distribution ===
if exist "%DIST%" rmdir /s /q "%DIST%"
mkdir "%DIST%"
mkdir "%DIST%\shaders"

copy /y "%BLD%\VoxelEngine.exe" "%DIST%\"
copy /y "%SRC%\shaders\*" "%DIST%\shaders\"
echo 127.0.0.1 > "%DIST%\server.txt"

(
echo VoxelEngine - Standalone Release
echo =================================
echo.
echo How to Play:
echo 1. Double-click VoxelEngine.exe to launch!
echo.
echo Controls:
echo - WASD: Move / Walk
echo - Space: Jump [Survival] or Fly Up [Creative]
echo - Left Ctrl: Sprint
echo - Left Click: Break Block
echo - Right Click: Place Block [1-5 to select block]
echo - F: Toggle between Survival Walking and Creative Flight
echo - Escape: Open Pause / Settings Menu
echo - Q: Quit
echo.
echo Multiplayer:
echo - To join the host, ensure the host IP is set in server.txt, then click 'CONNECT TO SERVER' in the Escape menu [or press J].
echo - To host your own server, press H or click 'HOST SERVER' in the Escape menu.
) > "%DIST%\HOW_TO_PLAY.txt"

powershell -Command "Compress-Archive -Path '%DIST%\*' -DestinationPath '%SRC%\VoxelEngine-Windows-x64.zip' -Force"

echo.
echo === Packaging Complete! ===
echo Ready-to-share ZIP created at:
echo %SRC%\VoxelEngine-Windows-x64.zip

endlocal