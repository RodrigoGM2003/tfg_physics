#version 430 core

#include "common_structs.glsl"

layout(std430, binding = 3) buffer AABBsBuffer {
    AABBStruct aabbs[];
};

layout(std430, binding = 4) buffer ResultsBuffer {
    int results[];
};

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

bool checkAABBCollision(AABBStruct a, AABBStruct b) {
    // Check for overlap on all axes
    return (a.min.x <= b.max.x && a.max.x >= b.min.x) &&
           (a.min.y <= b.max.y && a.max.y >= b.min.y) &&
           (a.min.z <= b.max.z && a.max.z >= b.min.z);
}

void main() {
    uint gid = gl_GlobalInvocationID.x;
    
    if (gid >= aabbs.length()) return;
    
    // Reset collision counter for this object
    results[gid] = 0;
    
    // Get the current AABB
    AABBStruct current = aabbs[gid];
    
    // Check against all other AABBs
    for (uint i = 0; i < aabbs.length(); i++) {
        if (i == gid) continue; // Skip self
        
        if (checkAABBCollision(current, aabbs[i])) {
            // Increment collision counter
            results[gid] = results[gid] + 1;
        }
    }
}