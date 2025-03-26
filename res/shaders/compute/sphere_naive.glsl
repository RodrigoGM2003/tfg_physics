#version 430 core

#include "common_structs.glsl"

layout(std430, binding = 7) buffer SphereBuffer {
    vec4 spheres[];
};

layout(std430, binding = 4) buffer ResultsBuffer {
    int results[];
};

layout(local_size_x = 512, local_size_y = 1, local_size_z = 1) in;

bool checkSphereCollision(vec4 a, vec4 b) {
    vec3 centerDiff = a.xyz - b.xyz;
    float distSquared = dot(centerDiff, centerDiff);
    float radiusSumSquared = (a.w + b.w) * (a.w + b.w);
    return distSquared <= radiusSumSquared;
}

void main() {
    uint gid = gl_GlobalInvocationID.x;

    // Early exit if out of bounds
    if (gid >= spheres.length()) return;

    results[gid] = 0;

    vec4 current = spheres[gid];
    int collisionCount = 0;
    
    //Full brute-force approach with reduced memory complexity
    for (uint i = 0; i < spheres.length(); i++) {
        if (i == gid) continue; // Skip self
        
        if (checkSphereCollision(current, spheres[i])) {
            collisionCount++;
        }
    }
    results[gid] = collisionCount;
}