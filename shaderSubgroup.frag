#version 450
#extension GL_KHR_shader_subgroup_basic : enable

layout(location = 0) out vec4 outColor;

void main() {
    uint idx = gl_SubgroupInvocationID;

    // Map the subgroup index to a color for visualization
    // Just a simple coloring by splitting idx into RGB channels
    float r = float((idx & 0x1) >> 0);
    float g = float((idx & 0x2) >> 1);
    float b = float((idx & 0x4) >> 2);

    outColor = vec4(r, g, b, 1.0);
}
