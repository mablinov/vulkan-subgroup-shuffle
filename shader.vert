// shader.vert
// Vertex shader to generate a full-screen triangle.
// It doesn't take any inputs and generates vertices based on gl_VertexIndex.
#version 450

// We pass the fragment coordinate to the fragment shader.
layout(location = 0) out vec2 outFragCoord;

void main() {
    // Generate a triangle that covers the entire screen.
    // This is a common trick to avoid using a VBO for a full-screen pass.
    float x = -1.0 + float((gl_VertexIndex & 1) << 2);
    float y = -1.0 + float((gl_VertexIndex & 2) << 1);
    
    // Pass the screen-space coordinates to the fragment shader
    outFragCoord = vec2(x, y);

    gl_Position = vec4(x, y, 0.0, 1.0);
}
