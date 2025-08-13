# build.ps1
# A simple PowerShell script to compile the Vulkan demo on Windows.
#
# Requirements:
# 1. Visual Studio C++ compiler (cl.exe).
#    - Run this from a "Developer Command Prompt for VS".
# 2. Vulkan SDK installed.
#    - The script assumes VULKAN_SDK is set as an environment variable.

# compile.ps1
# This script compiles the shaders and the C code.

Write-Host "Compiling shaders..."

# Run glslc for vertex shader
& "$env:VULKAN_SDK\bin\glslc.exe" --target-spv=spv1.3 shader.vert -o spv/shader.vert.spv
if ($LASTEXITCODE -ne 0) {
    Write-Error "Vertex shader compilation failed."
    exit 1
}

# Run glslc for fragment shader
& "$env:VULKAN_SDK\bin\glslc.exe" shader.frag -o spv/shader.frag.spv
& "$env:VULKAN_SDK\bin\glslc.exe" shaderSubgroup.frag -o spv/shaderSubgroup.frag.spv
& "$env:VULKAN_SDK\bin\glslc.exe" shaderSubgroupGray.frag -o spv/shaderSubgroupGray.frag.spv
& "$env:VULKAN_SDK\bin\glslc.exe" --target-spv=spv1.3 shaderSubgroupShuffleGray.frag -o spv/shaderSubgroupShuffleGray.frag.spv
& "$env:VULKAN_SDK\bin\glslc.exe" --target-spv=spv1.3 shaderShuffle.frag -o spv/shaderShuffle.frag.spv
if ($LASTEXITCODE -ne 0) {
    Write-Error "Fragment shader compilation failed."
    exit 1
}
Write-Host "Shaders compiled successfully."

# Check if cl.exe is available
if (-not (Get-Command cl.exe -ErrorAction SilentlyContinue)) {
    Write-Host "ERROR: cl.exe not found in PATH."
    Write-Host "Please run this script from a 'Developer Command Prompt for VS'."
    exit 1
}

# Check if VULKAN_SDK is set
if (-not $env:VULKAN_SDK) {
    Write-Host "ERROR: VULKAN_SDK environment variable is not set."
    Write-Host "Please install the Vulkan SDK from https://vulkan.lunarg.com/ and ensure the environment variable is configured."
    exit 1
}

Write-Host "--- Compiling Vulkan Demo ---"

# Compile the C code, including the Vulkan SDK headers and linking the Vulkan library
cl.exe main.c /I"$env:VULKAN_SDK\Include" /link /LIBPATH:"$env:VULKAN_SDK\Lib" vulkan-1.lib /OUT:render.exe
cl.exe compute.c /I"$env:VULKAN_SDK\Include" /link /LIBPATH:"$env:VULKAN_SDK\Lib" vulkan-1.lib /OUT:compute.exe

if ($LASTEXITCODE -ne 0) {
    Write-Host ""
    Write-Host "--- Build FAILED ---"
    exit $LASTEXITCODE
} else {
    Write-Host ""
    Write-Host "--- Build SUCCEEDED ---"
    Write-Host "Run 'render.exe' to generate 'render.ppm'."
}
