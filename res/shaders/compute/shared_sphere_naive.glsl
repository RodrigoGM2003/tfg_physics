#version 430 core

#include "common_structs.glsl"

layout(std430, binding = 7) buffer SphereBuffer {
    vec4 spheres[];
};

layout(std430, binding = 4) buffer ResultsBuffer {
    int results[];
};

// Increase workgroup size for better occupancy on modern GPUs
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

// Shared memory to cache AABBs for the current workgroup
shared vec4 sharedSpheres[256]; // Match with local_size_x
shared uint numSpheres;

void main() {
    uint gid = gl_GlobalInvocationID.x;
    uint lid = gl_LocalInvocationID.x;
    
    // Get total number of AABBs once per workgroup
    if (lid == 0) {
        numSpheres = spheres.length();
    }
    
    // Wait for numSpheres to be initialized
    barrier();
    memoryBarrierShared();
    
    // Early exit if out of bounds
    if (gid >= numSpheres) return;
    
    results[gid] = 0;
    
    // Store current AABB
    vec4 current = spheres[gid];
    
    // Initialize collision counter
    int collisionCount = 0;
    
    // Process AABBs in chunks to utilize shared memory
    const uint CHUNK_SIZE = 256; // Match with local_size_x
    for (uint chunkStart = 0; chunkStart < numSpheres; chunkStart += CHUNK_SIZE) {
        // Load chunk into shared memory cooperatively
        // if (chunkStart + lid < numSpheres) {
        //     sharedSpheres[lid] = spheres[chunkStart + lid];
        // }
        sharedSpheres[lid] = chunkStart + lid < numSpheres ? spheres[chunkStart + lid] : vec4(999999.0, 999999.0, 999999.0, 0.0);
        // Ensure all data is loaded
        
        barrier();
        memoryBarrierShared();
        
        // Process chunk
        uint chunkEnd = min(chunkStart + CHUNK_SIZE, numSpheres);
        for (uint i = 0; i < chunkEnd - chunkStart; i++) {
            uint index = chunkStart + i;
            vec4 other = sharedSpheres[i];
            float r = current.w + other.w;
            collisionCount += (index != gid && dot(current.xyz - other.xyz, current.xyz - other.xyz) <= r * r) ? 1 : 0;
            // if (index == gid) continue; // Skip self
            
            // if (checkAABBCollision(current, sharedSpheres[i])) {
            //     collisionCount++;
            // }
        }
        
        // Wait for all threads to finish with shared memory
        barrier();
        memoryBarrierShared();
    }
    
    // Write result once
    results[gid] = collisionCount;
}