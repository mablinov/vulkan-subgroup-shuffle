#version 450
#extension GL_KHR_shader_subgroup_basic : enable
#extension GL_KHR_shader_subgroup_basic : enable
#extension GL_KHR_shader_subgroup_shuffle : enable

layout(location = 0) out vec4 outColor;

void main() {
    uint idx = gl_SubgroupInvocationID;
    uint size = gl_SubgroupSize;
    uint targetIndex = size - 1 - gl_SubgroupInvocationID;

    // Normalize index to [0,1] range
    float gray = float(idx) / float(size - 1);
    gray = subgroupShuffle(gray, targetIndex);

    outColor = vec4(gray, gray, gray, 1.0);
}
