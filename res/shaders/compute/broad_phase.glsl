#version 430 core

#include "common_structs.glsl"

layout(std430, binding = 3) buffer AABBsBuffer {
    AABBStruct aabbs[];
};


layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

uniform ivec3 grid_size; // (Nx, Ny, Nz) total cells

int hashObject(vec3 position) {
    const float cell_size = 1.0; // Adjust grid resolution
    ivec3 cell = ivec3(floor(position / cell_size));

    // Ensure cell is within bounds
    cell = clamp(cell, ivec3(0), grid_size - ivec3(1));

    // Convert (x, y, z) to a 1D array index
    return cell.x + grid_size.x * (cell.y + grid_size.y * cell.z);
}

void main(){
    uint gid = gl_GlobalInvocationID.x;

    if (gid >= aabbs.length()) return;

    int key = hashObject(aabbs[gid].center.xyz);

    // grid_hashes[gid] = hash_key;
    // object_indices[gid] = int(gid); 
}