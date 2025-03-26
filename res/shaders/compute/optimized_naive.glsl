#version 430 core

#include "common_structs.glsl"

layout(std430, binding = 3) buffer AABBsBuffer {
    AABBStruct aabbs[];
};

layout(std430, binding = 4) buffer ResultsBuffer {
    int results[];
};

// Increase workgroup size for better occupancy on modern GPUs
layout(local_size_x = 512, local_size_y = 1, local_size_z = 1) in;

// Shared memory to cache AABBs for the current workgroup
shared AABBStruct sharedAABBs[512]; // Match with local_size_x
shared uint numAABBs;

bool checkAABBCollision(AABBStruct a, AABBStruct b) {
    // Early out if any axis doesn't overlap
    if (a.min.x > b.max.x || a.max.x < b.min.x) return false;
    if (a.min.y > b.max.y || a.max.y < b.min.y) return false;
    if (a.min.z > b.max.z || a.max.z < b.min.z) return false;
    
    return true;
}

void main() {
    uint gid = gl_GlobalInvocationID.x;
    uint lid = gl_LocalInvocationID.x;
    
    // Get total number of AABBs once per workgroup
    if (lid == 0) {
        numAABBs = aabbs.length();
    }
    
    // Wait for numAABBs to be initialized
    barrier();
    memoryBarrierShared();
    
    // Early exit if out of bounds
    if (gid >= numAABBs) return;
    
    results[gid] = 0;
    
    // Store current AABB
    AABBStruct current = aabbs[gid];
    
    // Initialize collision counter
    int collisionCount = 0;
    
    // Process AABBs in chunks to utilize shared memory
    const uint CHUNK_SIZE = 512; // Match with local_size_x
    for (uint chunkStart = 0; chunkStart < numAABBs; chunkStart += CHUNK_SIZE) {
        // Load chunk into shared memory cooperatively
        if (chunkStart + lid < numAABBs) {
            sharedAABBs[lid] = aabbs[chunkStart + lid];
        }
        
        // Ensure all data is loaded
        barrier();
        memoryBarrierShared();
        
        // Process chunk
        uint chunkEnd = min(chunkStart + CHUNK_SIZE, numAABBs);
        for (uint i = 0; i < chunkEnd - chunkStart; i++) {
            uint index = chunkStart + i;
            if (index == gid) continue; // Skip self
            
            if (checkAABBCollision(current, sharedAABBs[i])) {
                collisionCount++;
            }
        }
        
        // Wait for all threads to finish with shared memory
        barrier();
        memoryBarrierShared();
    }
    
    // Write result once
    results[gid] = collisionCount;
}