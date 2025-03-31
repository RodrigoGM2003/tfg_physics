#version 430 core

#include "common_structs.glsl"

layout(std430, binding = 7) buffer SphereBuffer {
    vec4 spheres[];
};

layout(std430, binding = 4) buffer ResultsBuffer {
    int results[];
};

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

shared vec4 sharedSpheres[256]; // Match with local_size_x
shared uint numSpheres;

void main() {
    uint gid = gl_GlobalInvocationID.x / 64;
    uint lid = gl_LocalInvocationID.x;
    uint start = gl_GlobalInvocationID.x % 64;

    // Early exit if out of bounds
    if (gid >= spheres.length()) return;

    if(start == 0){
        results[gid] = 0;
    }
    if(lid == 0){
        numSpheres = spheres.length();
    }
    barrier();

    vec4 current = spheres[gid];
    int collisionCount = 0;

    vec3 cPos = current.xyz;
    float cr = current.w;
    for (uint i = start; i < numSpheres; i+=64) {      
        vec4 other = spheres[i];
        float r = cr + other.w;
        collisionCount += (i != gid && dot(cPos - other.xyz, cPos - other.xyz) <= r * r) ? 1 : 0;
    }

    if(collisionCount > 0){
        atomicAdd(results[gid], collisionCount);
    }
}