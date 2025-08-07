@echo off
REM build.bat
REM A simple batch file to compile the Vulkan demo on Windows.
REM
REM Requirements:
REM 1. Visual Studio C++ compiler (cl.exe).
REM    - Run this from a "Developer Command Prompt for VS".
REM 2. Vulkan SDK installed.
REM    - The script assumes VULKAN_SDK is set as an environment variable.

REM Check if cl.exe is available
where cl >nul 2>nul
if %errorlevel% neq 0 (
    echo ERROR: cl.exe not found in PATH.
    echo Please run this script from a "Developer Command Prompt for VS".
    goto :eof
)

REM Check if VULKAN_SDK is set
if not defined VULKAN_SDK (
    echo ERROR: VULKAN_SDK environment variable is not set.
    echo Please install the Vulkan SDK from https://vulkan.lunarg.com/ and ensure the environment variable is configured.
    goto :eof
)

echo --- Compiling Vulkan Demo ---

REM Compile the C code, including the Vulkan SDK headers and linking the Vulkan library.
cl.exe main.c /I"%VULKAN_SDK%\Include" /link /LIBPATH:"%VULKAN_SDK%\Lib" vulkan-1.lib /OUT:render.exe

if %errorlevel% neq 0 (
    echo.
    echo --- Build FAILED ---
) else (
    echo.
    echo --- Build SUCCEEDED ---
    echo Run 'render.exe' to generate 'render.ppm'.
)

:eof
