#version 430 core

#include "common_structs.glsl"

layout(std430, binding = 7) buffer SphereBuffer {
    vec4 spheres[];
};

layout(std430, binding = 4) buffer ResultsBuffer {
    int results[];
};

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main() {
    uint gid = gl_GlobalInvocationID.x / 64;
    uint start = gl_GlobalInvocationID.x % 64;

    // Early exit if out of bounds
    if (gid >= spheres.length()) return;

    if(start == 0){
        results[gid] = 0;
    }
    barrier();

    vec4 current = spheres[gid];
    int collisionCount = 0;

    for (uint i = start; i < spheres.length(); i+=64) {      
        vec4 other = spheres[i];
        float r = current.w + other.w;
        collisionCount += (i != gid && dot(current.xyz - other.xyz, current.xyz - other.xyz) <= r * r) ? 1 : 0;
    }

    if(collisionCount > 0){
        atomicAdd(results[gid], collisionCount);
    }
}