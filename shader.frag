// shader.frag
// Fragment shader that colors pixels based on their screen coordinates.
#version 450

// The final output color for the pixel.
layout(location = 0) out vec4 outColor;

void main() {
    // gl_FragCoord contains the window-relative coordinates of the fragment.
    // The .xy components are the pixel coordinates.
    // We normalize them to the [0, 1] range.
    float r = gl_FragCoord.x / 256.0;
    float g = gl_FragCoord.y / 256.0;
    
    // Output a color based on the normalized coordinates.
    // Red channel increases from left to right.
    // Green channel increases from top to bottom.
    // Blue channel is constant.
    outColor = vec4(r, g, 0.2, 1.0);
}
