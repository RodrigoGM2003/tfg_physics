#version 430 core

#include "common_structs.glsl"

layout(std430, binding = 4) buffer ResultsBuffer {
    int results[];
};

layout(std430, binding = 7) buffer SphereBuffer {
    vec4 spheres[];
};

layout(std430, binding = 8) buffer CollisionPairsBuffer {
    ivec2 collision_pairs[];
    uint collision_count;
};

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main() {
    uint gid = gl_GlobalInvocationID.x / 64;
    uint start = gl_GlobalInvocationID.x % 64;

    // Early exit if out of bounds
    if (gid >= spheres.length()) return;

    if(start == 0){
        results[gid] = 0;
        // Initialize collision count if this is the first invocation
        if(gid == 0) collision_count = 0;
    }
    barrier();

    vec4 current = spheres[gid];

    for (uint i = start; i < spheres.length(); i+=64) {
        if(i <= gid) continue; // Only check each pair once
        
        vec4 other = spheres[i];
        float r = current.w + other.w;
        
        if(dot(current.xyz - other.xyz, current.xyz - other.xyz) <= r * r) {
            // This is a potential collision
            atomicAdd(results[gid], 1);
            
            // Add collision pair to the buffer
            uint index = atomicAdd(collision_count, 1);
            if(index < collision_pairs.length()) {
                collision_pairs[index] = ivec2(gid, i);
            }
        }
    }
}