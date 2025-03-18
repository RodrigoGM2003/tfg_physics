#version 430 core

layout(std430, binding = 1) buffer TransformBuffer {
    mat4 transforms[];
};

struct PropertiesStruct {
    vec4 velocity;
    vec4 acceleration;
    vec4 angular_velocity;
    vec4 angular_acceleration;
};
layout(std430, binding = 2) buffer PropertiesBuffer {
    PropertiesStruct properties[];
};

struct AABBStruct {
    vec4 min;
    vec4 max;
    vec4 center;
    vec4 extents;
};
layout(std430, binding = 3) buffer AABBsBuffer {
    AABBStruct aabbs[];
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
    
    // Early exit - move this before any computation
    if (gid >= transforms.length()) return;
    
    // Cache property data - read once
    PropertiesStruct prop = properties[gid];
    vec3 velocity = prop.velocity.xyz;
    vec3 angular_vel = prop.angular_velocity.xyz;
    
    // Extract transform components
    mat4 transform = transforms[gid];
    vec3 position = transform[3].xyz;
    mat3 rotation = mat3(
        transform[0].xyz,
        transform[1].xyz,
        transform[2].xyz
    );
    
    // Update position (simple integration)
    vec3 new_position = position + velocity * delta_time;
    
    // Update rotation using optimized method
    mat3 new_rotation = updateRotation(rotation, angular_vel, delta_time);
    
    // Rebuild transform matrix efficiently
    transforms[gid] = mat4(
        vec4(new_rotation[0], 0.0),
        vec4(new_rotation[1], 0.0),
        vec4(new_rotation[2], 0.0),
        vec4(new_position, 1.0)
    );
    
    // Efficient AABB update - only update what's needed
    AABBStruct aabb = aabbs[gid];
    vec3 center = new_position;
    vec3 extents = aabb.extents.xyz;
    
    aabbs[gid].center.xyz = center;
    aabbs[gid].min.xyz = center - extents;
    aabbs[gid].max.xyz = center + extents;
}