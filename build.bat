@echo off
echo === COD FPS Build Script ===
echo.

REM Add MinGW and CMake to PATH
set PATH=C:\msys64\mingw64\bin;C:\msys64\usr\bin;C:\Program Files\CMake\bin;%PATH%

REM Configure
echo [1/2] Configuring with CMake...
cmake -B build -G "MinGW Makefiles" -DCMAKE_BUILD_TYPE=Release -DCMAKE_POLICY_VERSION_MINIMUM=3.5
if errorlevel 1 (
    echo.
    echo ERROR: CMake configuration failed!
    echo Make sure you have CMake and MinGW-w64 installed via MSYS2.
    echo Run: pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake
    pause
    exit /b 1
)

REM Build
echo.
echo [2/2] Building...
cmake --build build --config Release
if errorlevel 1 (
    echo.
    echo ERROR: Build failed!
    pause
    exit /b 1
)

echo.
echo === BUILD SUCCESSFUL ===
echo Run 'run.bat' to start the game!
echo.
pause
