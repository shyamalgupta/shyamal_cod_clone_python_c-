@echo off
echo === COD FPS - Starting Game ===
echo.

REM Add MinGW to PATH for DLL access
set PATH=C:\msys64\mingw64\bin;%PATH%

if exist "build\COD_FPS.exe" (
    echo Starting COD FPS...
    echo.
    start "" "build\COD_FPS.exe"
) else (
    echo ERROR: Game executable not found!
    echo Run 'build.bat' first to compile the game.
    echo.
    pause
)
