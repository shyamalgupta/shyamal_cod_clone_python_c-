@echo off
echo ========================================
echo   COD FPS - Python + OpenGL Edition
echo ========================================
echo.
echo Launching game... (Procedural Textures and OpenGL Fog Enabled)
python "%~dp0cod_fps.py"
if %ERRORLEVEL% NEQ 0 (
    echo.
    echo [ERROR] Game crashed or dependencies missing!
    echo Run: pip install pygame PyOpenGL PyOpenGL_accelerate numpy
    pause
)
