#version 440 core

#include "common_structs.glsl"


layout(std430, binding = 3) buffer AABBsBuffer {
    AABBStruct aabbs[];
};

layout(std430, binding = 4) buffer ResultsBuffer {
    int results[];
};


layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;


bool intersect(AABBStruct a1, AABBStruct a2){
    return (a1.min.x <= a2.max.x && a1.max.x >= a2.min.x) &&
           (a1.min.y <= a2.max.y && a1.max.y >= a2.min.y) &&
           (a1.min.z <= a2.max.z && a1.max.z >= a2.min.z);
}

void main() {
    uint gid = gl_GlobalInvocationID.x;
    
    int a = 0;
    // Early exit
    if (gid >= aabbs.length()) return;

    // Traverse all items in the AABB array and check for intersections
    for (uint i = 0; i < aabbs.length(); i++) {
        if (i != gid && intersect(aabbs[gid], aabbs[i])) {
            // Handle intersection
            // For example, you can set a flag or perform some action
            a += 1;
        }
    }

    results[gid] = a;
}