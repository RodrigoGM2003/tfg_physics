#version 430 core

#include "common_structs.glsl"

layout(std430, binding = 3) buffer AABBsBuffer {
    AABBStruct aabbs[];
};

layout(std430, binding = 4) buffer MortonCodes {
    uint morton[];
};

uniform vec3 world_max;
uniform vec3 world_min;

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;


uint expandBits(uint n){
    n = (n * 0x00010001u) & 0xFF0000FFu;
    n = (n * 0x00000101u) & 0x0F00F00Fu;
    n = (n * 0x00000011u) & 0xC30C30C3u;
    n = (n * 0x00000005u) & 0x49249249u;
    return n;
}

uint morton3d(float x, float y, float z){
    // Scale to [0,1023] range and clamp
    x = min(max(x * 1024.0f, 0.0f), 1023.0f);
    y = min(max(y * 1024.0f, 0.0f), 1023.0f);
    z = min(max(z * 1024.0f, 0.0f), 1023.0f);

    // Expand bits
    uint xx = expandBits(uint(x));
    uint yy = expandBits(uint(y));
    uint zz = expandBits(uint(z));

    // Properly interleave bits (not add with weights)
    return xx | (yy << 1) | (zz << 2);
}

void main() {
    uint gid = gl_GlobalInvocationID.x;
    
    // Early exit
    if (gid >= aabbs.length()) return;

    float max_extent = max(max(world_max.x - world_min.x, world_max.y - world_min.y), world_max.z - world_min.z);
    vec3 world_scale_inv = vec3(1.0 / max_extent);

    vec3 normalized_center = (aabbs[gid].center.xyz - world_min) * world_scale_inv;


    morton[gid] = morton3d(normalized_center.x, normalized_center.y, normalized_center.z);
}