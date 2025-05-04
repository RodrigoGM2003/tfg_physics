#version 440 core
#extension GL_NV_shader_atomic_float : enable

#include "common_structs.glsl"

layout(std430, binding = 5) buffer PropertiesBuffer {
    PropertiesStruct properties[];
};

layout(std430, binding = 21) buffer CollisionCountBuffer{
    uint collisionCount;
};

// The ContactManifold struct definition remains the same
struct ContactManifold {
    uint indexA;
    uint indexB;
    vec4 normal;          // World space, consistent direction (e.g., A->B)
    float depth;          // Penetration depth
};

// Add the new output buffer for manifolds
layout(std430, binding = 26) buffer ContactManifoldBuffer {
    ContactManifold manifolds[];
};

layout(std430, binding = 29) buffer DeltaVBuffer {
    vec4 deltaVs[];
};

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;


void main(){
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= collisionCount) return;


    ContactManifold contact = manifolds[gid];

    // Get positions from transforms (assuming column 3 contains translation)
    // Use the normalized vector from o1 to o2 as an approximation of the collision normal
    vec3 collisionNormal = contact.normal.xyz;
    float depth = contact.depth;
    // vec3 collisionNormal = normalize(pos2 - pos1);

    // Retrieve velocities and inverse masses from the properties buffer
    vec3 v1 = properties[contact.indexA].velocity.xyz;
    vec3 v2 = properties[contact.indexB].velocity.xyz;
    float invMass1 = properties[contact.indexA].inverseMass;
    float invMass2 = properties[contact.indexB].inverseMass;

    // Calculate relative velocity along the collision normal
    vec3 relVel = v2 - v1;
    float relVelAlongNormal = dot(relVel, collisionNormal);

    // Only apply impulse if objects are moving towards each other
    if (relVelAlongNormal < 0.0 && depth > 0.01) {
        float restitution = 0.5; // Coefficient of restitution (bounciness)

        // Calculate impulse scalar using the formula:
        // j = -(1 + restitution) * (v_rel · n) / (invMass1 + invMass2)
        float j = -(1.0 + restitution) * relVelAlongNormal / (invMass1 + invMass2);
        vec3 impulse = j > 0 ? j * collisionNormal : vec3(0.0f);

        // Update velocities with the impulse (Newton's third law)
        // v1 -= impulse * invMass1;
        // v2 += impulse * invMass2;
        vec3 deltaVA = -impulse * invMass1;
        vec3 deltaVB = impulse * invMass2; 


        atomicAdd(deltaVs[contact.indexA].x, deltaVA.x);
        atomicAdd(deltaVs[contact.indexA].y, deltaVA.y);
        atomicAdd(deltaVs[contact.indexA].z, deltaVA.z);

        atomicAdd(deltaVs[contact.indexB].x, deltaVB.x);
        atomicAdd(deltaVs[contact.indexB].y, deltaVB.y);
        atomicAdd(deltaVs[contact.indexB].z, deltaVB.z);


        // properties[contact.indexA].velocity = vec4(v1, 0.0);
        // properties[contact.indexB].velocity = vec4(v2, 0.0);
    }

    // ContactManifold blankContact;
    // blankContact.indexA = 0;
    // blankContact.indexB = 0;
    // blankContact.normal = vec4(0.0f);
    // blankContact.depth = 0.0f;
    // manifolds[gid] = blankContact;
}