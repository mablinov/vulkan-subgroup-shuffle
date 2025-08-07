#version 450
#extension GL_KHR_shader_subgroup_basic : enable
#extension GL_KHR_shader_subgroup_shuffle : enable

layout(location = 0) out vec4 outColor;

void main() {
    // Get the screen-relative coordinates.
    float r = gl_FragCoord.x / 256.0;
    float g = gl_FragCoord.y / 256.0;
    vec4 color = vec4(r, g, 0.2, 1.0);

    // Convert the float color to uint for bitwise-safe shuffling.
    uvec4 packed = floatBitsToUint(color);

    // Reverse the pixels within the subgroup.
    uint subgroupSize = gl_SubgroupSize;
    uint targetIndex = subgroupSize - 1 - gl_SubgroupInvocationID;

    // Shuffle each component of the packed color vector.
    uvec4 shuffledPacked;
    shuffledPacked.r = subgroupShuffle(packed.r, targetIndex);
    shuffledPacked.g = subgroupShuffle(packed.g, targetIndex);
    shuffledPacked.b = subgroupShuffle(packed.b, targetIndex);
    shuffledPacked.a = subgroupShuffle(packed.a, targetIndex);

    // Convert back to float and write the shuffled color.
    outColor = uintBitsToFloat(shuffledPacked);
}
