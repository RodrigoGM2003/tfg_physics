#version 430 core

#include "common_structs.glsl"

layout(std430, binding = 4) buffer ResultsBuffer {
    int results[];
};

layout(std430, binding = 7) buffer SphereBuffer {
    vec4 spheres[];
};

layout(std430, binding = 20) buffer CollisionPairsBuffer {
    ivec2 collisionPairs[];
};

layout(std430, binding = 21) buffer CollisionCountBuffer{
    uint collisionCount;
};

layout(std430, binding = 22) buffer SecondResultsBuffer{
    uint second_results[];
};



layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;


void main() {
    uint gid = gl_GlobalInvocationID.x;


    // Early exit if out of bounds
    if (gid >= spheres.length()) return;

    results[gid] = 0;
    second_results[gid] = 0;

    // Initialize collision count if this is the first invocation
    if(gid == 0) collisionCount = 0;
    barrier();

    vec4 current = spheres[gid];
    for (uint i = gid + 1; i < spheres.length(); i++) {
        vec4 other = spheres[i];
        float r = current.w + other.w;

        float distSquared = dot(current.xyz - other.xyz, current.xyz - other.xyz);
        bool isCollision = distSquared <= r * r;
        
        if(dot(current.xyz - other.xyz, current.xyz - other.xyz) <= r * r) {
            // Add collision pair to the buffer
            uint index = atomicAdd(collisionCount, 1);
            if(index < collisionPairs.length()) {
                collisionPairs[index] = ivec2(gid, i);
            }
        }
    }
}