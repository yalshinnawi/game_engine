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
echo ================================================
echo           VoxelEngine - Standalone
echo ================================================
echo.
echo HOW TO PLAY:
echo 1. Double-click VoxelEngine.exe to launch!
echo.
echo CONTROLS:
echo - WASD: Move / Walk around
echo - Mouse: Look around [captured automatically]
echo - Left Click: Break targeted block
echo - Right Click: Place selected block
echo - 1 to 5 or Scroll Wheel: Switch block material
echo - Space: Jump [Survival] / Fly Up [Creative]
echo - Left Ctrl: Sprint
echo - F: Toggle between Survival Walking and Creative Flying
echo - Escape: Open Pause / Settings Menu
echo - Q: Quit the game
echo.
echo MULTIPLAYER SETUP:
echo 1. Firewall: Host must allow VoxelEngine on Private AND Public networks in Windows Firewall.
echo 2. Hamachi: Both join the same Hamachi room. Put the Host's 25.x.x.x IP into server.txt.
echo 3. Host: Host launches game, presses Escape and clicks [ HOST SERVER ] [or press key H].
echo 4. Client: Friend launches game, presses Escape and clicks [ CONNECT ] [or press key J].
echo.
echo If a connection fails, check in-game error banner and network_log.txt for details!
) > "%DIST%\HOW_TO_PLAY.txt"

powershell -Command "Compress-Archive -Path '%DIST%\*' -DestinationPath '%SRC%\VoxelEngine-Windows-x64.zip' -Force"

echo.
echo === Packaging Complete! ===
echo Ready-to-share ZIP created at:
echo %SRC%\VoxelEngine-Windows-x64.zip

endlocal