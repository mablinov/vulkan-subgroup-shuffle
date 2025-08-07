#version 450
#extension GL_KHR_shader_subgroup_basic : enable

layout(location = 0) out vec4 outColor;

void main() {
    uint idx = gl_SubgroupInvocationID;
    uint size = gl_SubgroupSize;

    // Normalize index to [0,1] range
    float gray = float(idx) / float(size - 1);

    outColor = vec4(gray, gray, gray, 1.0);
}
