#version 440 core // Need atomics and SSBOs

// Include structs (assuming PropertiesStruct and ContactManifold are defined)
#include "common_structs.glsl" 

layout(std430, binding = 5) buffer PropertiesBuffer {
    PropertiesStruct properties[];
};

layout(std430, binding = 29) buffer DeltaVBuffer {
    vec4 deltaVs[];
};

layout(std430, binding = 30) buffer DeltaWBuffer {
    vec4 deltaWs[];
};

// --- Main Shader Logic ---
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

void main(){
    uint gid = gl_GlobalInvocationID.x;

    if (gid >= properties.length())
        return;


    if(properties[gid].inverseMass > 0.0){
        properties[gid].velocity += deltaVs[gid];
        properties[gid].angular_velocity += deltaWs[gid];
    }

    // Reset accumulators for next iteration
    deltaVs[gid] = vec4(0.0);
    deltaWs[gid] = vec4(0.0);
}