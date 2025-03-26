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

// Boundary uniforms
uniform vec3 u_boundary_min = vec3(-27.5,-27.5,-27.5);  // Minimum boundary coordinates
uniform vec3 u_boundary_max = vec3(27.5,27.5,27.5);  // Maximum boundary coordinates
uniform float delta_time;
uniform float u_restitution = 1.0;  // Bounce factor, can be adjusted

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
    
    // Get the object's extents for boundary collision
    vec3 extents = aabbs[gid].extents.xyz;
    
    // Check and resolve boundary collisions for each axis separately
    vec3 collision_normal = vec3(0.0);
    bool collision_occurred = false;
    
    // // X-axis boundary collision
    // if (new_position.x - extents.x < u_boundary_min.x) {
    //     new_position.x = u_boundary_min.x + extents.x + 0.001; // Slight offset to prevent sticking
    //     collision_normal.x = 1.0;
    //     collision_occurred = true;
    // }
    // else if (new_position.x + extents.x > u_boundary_max.x) {
    //     new_position.x = u_boundary_max.x - extents.x - 0.001;
    //     collision_normal.x = -1.0;
    //     collision_occurred = true;
    // }
    
    // // Y-axis boundary collision
    // if (new_position.y - extents.y < u_boundary_min.y) {
    //     new_position.y = u_boundary_min.y + extents.y + 0.001;
    //     collision_normal.y = 1.0;
    //     collision_occurred = true;
    // }
    // else if (new_position.y + extents.y > u_boundary_max.y) {
    //     new_position.y = u_boundary_max.y - extents.y - 0.001;
    //     collision_normal.y = -1.0;
    //     collision_occurred = true;
    // }
    
    // // Z-axis boundary collision
    // if (new_position.z - extents.z < u_boundary_min.z) {
    //     new_position.z = u_boundary_min.z + extents.z + 0.001;
    //     collision_normal.z = 1.0;
    //     collision_occurred = true;
    // }
    // else if (new_position.z + extents.z > u_boundary_max.z) {
    //     new_position.z = u_boundary_max.z - extents.z - 0.001;
    //     collision_normal.z = -1.0;
    //     collision_occurred = true;
    // }
    
    // // Update velocity if collision occurred (reflection with restitution)
    // if (collision_occurred) {
    //     // Component-wise velocity reflection based on the collision normal
    //     velocity = velocity - 2.0 * dot(velocity, collision_normal) * collision_normal;
        
    //     // Apply restitution (energy loss)
    //     velocity *= u_restitution;
        
    //     // Store the updated velocity back
    //     prop.velocity.xyz = velocity;
    //     properties[gid].velocity = prop.velocity;
    // }
    
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
    extents = aabbs[gid].extents.xyz;
    aabbs[gid].center.xyz = new_position;
    aabbs[gid].min.xyz = new_position - extents;
    aabbs[gid].max.xyz = new_position + extents;
}