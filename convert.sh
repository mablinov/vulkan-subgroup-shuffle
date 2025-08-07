#!/bin/bash

# Convert if `convert` is available
if command -v convert &> /dev/null; then
    echo "Converting render.ppm to render.png..."
    convert render.ppm render.png
    if [ $? -eq 0 ]; then
        echo "Conversion successful! Output saved as render.png"
    else
        echo "Conversion failed."
    fi
else
    echo "ImageMagick's convert command not found. Skipping conversion to PNG."
fi
