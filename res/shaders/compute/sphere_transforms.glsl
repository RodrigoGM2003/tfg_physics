#version 430 core

#include "common_structs.glsl"

layout(std430, binding = 1) buffer TransformBuffer {
    mat4 transforms[];
};

layout(std430, binding = 5) buffer PropertiesBuffer {
    PropertiesStruct properties[];
};

layout(std430, binding = 4) buffer ResultsBuffer {
    int results[];
};

layout(std430, binding = 7) buffer SphereBuffer {
    vec4 spheres[];
};

layout(std430, binding = 29) buffer DeltaVBuffer {
    vec4 deltaVs[];
};

layout(std430, binding = 30) buffer DeltaWBuffer {
    vec4 deltaWs[];
};

uniform float delta_time;
uniform vec3 gravity = vec3(0.0f, -0.1f, 0.0f);
uniform float linearFriction = 0.00f;   // coefficient [1/s]
uniform float angularFriction = 0.00f;  // coefficient [1/s]

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

// Optimized rotation update for small angles using Rodrigues' rotation formula
mat3 updateRotation(mat3 R, vec3 omega, float dt) {
    float angle = length(omega) * dt;
    if (angle < 0.0001) {
        return R;
    }
    vec3 axis = omega / length(omega);
    float s = sin(angle);
    float c = cos(angle);
    float t = 1.0 - c;
    mat3 K = mat3(
        0, -axis.z, axis.y,
        axis.z, 0, -axis.x,
        -axis.y, axis.x, 0
    );
    mat3 rotMat = mat3(1.0) + s * K + t * (K * K);
    return rotMat * R;
}

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= transforms.length()) return;

    results[gid] = 0;

    // Load
    mat4 transform = transforms[gid];
    PropertiesStruct prop = properties[gid];
    vec3 velocity = prop.velocity.xyz;
    vec3 angular_vel = prop.angular_velocity.xyz;

    // Integrate linear
    if (prop.inverseMass != 0.0) {
        velocity += gravity * delta_time;
        // apply linear air friction: F_drag = -c * v
        float linFactor = 1.0 - linearFriction * delta_time;
        linFactor = max(linFactor, 0.0);
        velocity *= linFactor;
    }
    vec3 position = transform[3].xyz + velocity * delta_time;

    // Integrate angular
    if (prop.inverseMass != 0.0) {
        // apply angular air friction
        float angFactor = 1.0 - angularFriction * delta_time;
        angFactor = max(angFactor, 0.0);
        angular_vel *= angFactor;
    }

    // Update rotation
    mat3 rotation = mat3(transform);
    mat3 new_rotation;
    float omega_len = length(angular_vel);
    if (omega_len * delta_time < 0.0001) {
        new_rotation = rotation;
    } else {
        new_rotation = updateRotation(rotation, angular_vel, delta_time);
    }

    // Store back
    properties[gid].velocity.xyz = velocity;
    properties[gid].angular_velocity.xyz = angular_vel;
    transforms[gid] = mat4(
        vec4(new_rotation[0], 0.0),
        vec4(new_rotation[1], 0.0),
        vec4(new_rotation[2], 0.0),
        vec4(position,      1.0)
    );
    spheres[gid].xyz = position;
    deltaVs[gid] = vec4(0.0);
    deltaWs[gid] = vec4(0.0);
}