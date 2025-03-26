#version 430 core

#include "common_structs.glsl"

layout(std430, binding = 1) buffer TransformBuffer {
    mat4 transforms[];
};

layout(std430, binding = 2) buffer PropertiesBuffer {
    PropertiesStruct properties[];
};

layout(std430, binding = 3) buffer AABBsBuffer {
    AABBStruct aabbs[];
};

layout(std430, binding = 4) buffer ResultsBuffer {
    int results[];
};

layout(std430, binding = 7) buffer SphereBuffer {
    vec4 spheres[];
};

uniform float delta_time;

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

// Optimized rotation update for small angles using Rodrigues' rotation formula
mat3 updateRotation(mat3 R, vec3 omega, float dt) {
    float angle = length(omega) * dt;
    
    // Fast path for negligible rotation
    if (angle < 0.0001) {
        return R;
    }
    
    // Normalize axis only once
    vec3 axis = omega / length(omega);
    
    // Precompute sin and cos
    float s = sin(angle);
    float c = cos(angle);
    float t = 1.0 - c;
    
    // Build rotation matrix directly (Rodrigues' formula)
    mat3 K = mat3(
        0, -axis.z, axis.y,
        axis.z, 0, -axis.x,
        -axis.y, axis.x, 0
    );
    
    mat3 rotMat = mat3(1.0) + s * K + t * (K * K);
    
    // Apply to current rotation
    return rotMat * R;
}

void main() {
    uint gid = gl_GlobalInvocationID.x;
    
    // Early exit
    if (gid >= transforms.length()) return;
    
    // Cache all data we need in registers
    mat4 transform = transforms[gid];
    PropertiesStruct prop = properties[gid];
    vec3 velocity = prop.velocity.xyz;
    vec3 angular_vel = prop.angular_velocity.xyz;
    
    // Update position with simple integration
    vec3 position = transform[3].xyz;
    vec3 new_position = position + velocity * delta_time;
    
    // Optimized rotation update
    mat3 rotation = mat3(transform);
    float omega_len = length(angular_vel);
    
    mat3 new_rotation;
    if (omega_len * delta_time < 0.0001) {
        // Ultra-fast path for very small rotations
        new_rotation = rotation;
    } else {
        // Use the optimized rotation update
        new_rotation = updateRotation(rotation, angular_vel, delta_time);
    }
    
    // Write back transform in one operation
    transforms[gid] = mat4(
        vec4(new_rotation[0], 0.0),
        vec4(new_rotation[1], 0.0),
        vec4(new_rotation[2], 0.0),
        vec4(new_position, 1.0)
    );

        // Update AABB with rotation-invariant approach
    vec3 extents = aabbs[gid].extents.xyz;
    aabbs[gid].center.xyz = new_position;
    aabbs[gid].min.xyz = new_position - extents;
    aabbs[gid].max.xyz = new_position + extents;

    spheres[gid].xyz = new_position;
    // spheres[gid].w = max(max(extents.x, extents.y), extents.z);
}