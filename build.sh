#!/bin/bash
# compile.sh
# This script compiles the shaders and the C code.

# Ensure the Vulkan SDK is sourced, which provides glslc.
# For example: source /path/to/your/vulkansdk/1.x.x.x/setup-env.sh

# echo "Compiling shaders..."
# glslc shader.vert -o vert.spv
# glslc shader.frag -o frag.spv
# glslc shaderShuffle.frag -o frag.spv

# if [ $? -ne 0 ]; then
#     echo "Shader compilation failed."
#     exit 1
# fi

echo "Compiling C code..."
gcc -I1.4.321.1/x86_64/include/ -ggdb main.c -o render -lvulkan -ldl

if [ $? -ne 0 ]; then
    echo "C code compilation failed."
    exit 1
fi

echo ""
echo "Compilation successful!"
echo "Run with: ./render"
echo "Output will be saved to render.ppm"
