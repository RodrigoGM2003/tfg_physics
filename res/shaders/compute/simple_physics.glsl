#version 440 core
#extension GL_NV_shader_atomic_float : enable

// Include structs
#include "common_structs.glsl" 

// --- Uniforms ---
uniform float u_Timestep = 1.0 / 60.0;
uniform float u_Restitution = 1.0;     // Global restitution coefficient
uniform float u_Friction = 0.0;        // Global friction coefficient

// --- Buffer Definitions ---
struct ContactPoint {
    uint bodyA;
    uint bodyB;
    vec3 normal;          // World space, pointing A->B
    vec3 position;        // Contact point in world space
    float depth;          // Penetration depth
};

// Input Buffers
layout(std430, binding = 26) readonly buffer ContactBuffer {
    ContactPoint contacts[];
};

layout(std430, binding = 1) readonly buffer TransformBuffer {
    mat4 transforms[];
};

layout(std430, binding = 5) buffer PropertiesBuffer {
    PropertiesStruct properties[]; // Assume it has velocity, angular_velocity, inverseMass
};

// --- Helper Functions ---
mat3 getInverseInertiaTensorWorld(uint objectId) {
    mat3 R = mat3(transforms[objectId]); // Extract rotation part
    mat3 invI_local = properties[objectId].inverseInertiaTensor;
    return R * invI_local * transpose(R);
}

// --- Main Shader Logic ---
layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;
void main() {
    uint contactIndex = gl_GlobalInvocationID.x;
    
    if (contactIndex >= contacts.length()) {
        return;
    }
    
    // Get contact data
    ContactPoint contact = contacts[contactIndex];
    uint bodyA = contact.bodyA;
    uint bodyB = contact.bodyB;
    vec3 normal = contact.normal;
    vec3 position = contact.position;
    float depth = contact.depth;
    
    // Get body properties
    float invMassA = properties[bodyA].inverseMass;
    float invMassB = properties[bodyB].inverseMass;
    
    // Skip if both bodies are static
    if (invMassA <= 0.0 && invMassB <= 0.0) {
        return;
    }
    
    // Calculate position offset from center of mass
    vec3 posA = transforms[bodyA][3].xyz;
    vec3 posB = transforms[bodyB][3].xyz;
    vec3 rA = position - posA;
    vec3 rB = position - posB;
    
    // Get velocities
    vec3 velA = properties[bodyA].velocity.xyz;
    vec3 angVelA = properties[bodyA].angular_velocity.xyz;
    vec3 velB = properties[bodyB].velocity.xyz;
    vec3 angVelB = properties[bodyB].angular_velocity.xyz;
    
    // Calculate velocity at contact point
    vec3 contactVelA = velA + cross(angVelA, rA);
    vec3 contactVelB = velB + cross(angVelB, rB);
    vec3 relVel = contactVelB - contactVelA;
    
    // Calculate normal component of relative velocity
    float normalVel = dot(relVel, normal);
    
    // Skip if objects are separating
    if (normalVel > 0.0) {
        return;
    }
    
    // --- Simple Impulse Resolution ---
    
    // Calculate effective mass for collision
    mat3 invInertiaA = getInverseInertiaTensorWorld(bodyA);
    mat3 invInertiaB = getInverseInertiaTensorWorld(bodyB);
    
    vec3 angContribA = cross(invInertiaA * cross(rA, normal), rA);
    vec3 angContribB = cross(invInertiaB * cross(rB, normal), rB);
    
    float effectiveMass = invMassA + invMassB + dot(normal, angContribA + angContribB);
    
    if (effectiveMass <= 0.0) {
        return;
    }
    
    // Calculate impulse strength with simple restitution
    float j = -(1.0 + u_Restitution) * normalVel / effectiveMass;
    
    // Apply position correction for penetration (simple version)
    float positionCorrection = max(depth - 0.01, 0.0) * 0.2; // 20% correction with slop
    float positionImpulse = positionCorrection / effectiveMass;
    j += positionImpulse;
    
    // Calculate impulse vector
    vec3 impulse = j * normal;
    
    // Apply impulse to linear and angular velocities
    if (invMassA > 0.0) {
        // Apply linear impulse
        properties[bodyA].velocity.xyz -= impulse * invMassA;
        
        // Apply angular impulse
        vec3 angularImpulse = cross(rA, impulse);
        properties[bodyA].angular_velocity.xyz -= invInertiaA * angularImpulse;
        
        // Simple friction (tangential impulse)
        vec3 tangent = relVel - (normalVel * normal);
        float tangentLen = length(tangent);
        if (tangentLen > 0.001) {
            tangent = normalize(tangent);
            float frictionImpulse = u_Friction * j;
            vec3 frictionVector = -frictionImpulse * tangent;
            
            properties[bodyA].velocity.xyz -= frictionVector * invMassA;
            properties[bodyA].angular_velocity.xyz -= invInertiaA * cross(rA, frictionVector);
        }
    }
    
    if (invMassB > 0.0) {
        // Apply linear impulse
        properties[bodyB].velocity.xyz += impulse * invMassB;
        
        // Apply angular impulse
        vec3 angularImpulse = cross(rB, impulse);
        properties[bodyB].angular_velocity.xyz += invInertiaB * angularImpulse;
        
        // Simple friction (tangential impulse)
        vec3 tangent = relVel - (normalVel * normal);
        float tangentLen = length(tangent);
        if (tangentLen > 0.001) {
            tangent = normalize(tangent);
            float frictionImpulse = u_Friction * j;
            vec3 frictionVector = -frictionImpulse * tangent;
            
            properties[bodyB].velocity.xyz += frictionVector * invMassB;
            properties[bodyB].angular_velocity.xyz += invInertiaB * cross(rB, frictionVector);
        }
    }
}