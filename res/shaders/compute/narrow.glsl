#version 430 core

#include "common_structs.glsl"

layout(std430, binding = 1) buffer TransformBuffer {
    mat4 transforms[];
};

layout(std430, binding = 4) buffer ResultsBuffer {
    int results[];
};

layout(std430, binding = 20) buffer CollisionPairsBuffer {
    ivec2 collisionPairs[];
};

layout(std430, binding = 21) buffer CollisionCountBuffer{
    uint collisionCount;
};

layout(std430, binding = 22) buffer SecondResultsBuffer {
    int secondResults[];
};

layout(std430, binding = 23) buffer ObjectVerticesBuffer {
    vec4 objectVertices[];
};

layout(std430, binding = 24) buffer ObjectNormalsBuffer {
    vec4 objectNormals[];
};

layout(std430, binding = 25) buffer ObjectEdgesBuffer {
    vec4 objectEdges[];
};

layout(std430, binding = 5) buffer PropertiesBuffer {
    PropertiesStruct properties[];
};

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

struct NarrowObject {
    uint idx;
    mat4 transform;
};

struct Collision {
    bool colliding;
    vec3 axis;
    float depth;
};

Collision checkOverlap(vec3 axis, mat4 transform1, mat4 transform2, uint numVertices) {
    float min1 = dot(axis, (transform1 * objectVertices[0]).xyz);
    float max1 = min1;
    float min2 = dot(axis, (transform2 * objectVertices[0]).xyz);
    float max2 = min2;

    for (uint i = 1; i < numVertices; i++) {
        float proj1 = dot(axis, (transform1 * objectVertices[i]).xyz);
        min1 = min(min1, proj1);
        max1 = max(max1, proj1);
        float proj2 = dot(axis, (transform2 * objectVertices[i]).xyz);
        min2 = min(min2, proj2);
        max2 = max(max2, proj2);
    }

    Collision result;
    result.colliding = (min1 <= max2 && max1 >= min2);
    result.depth = result.colliding ? min(max2 - min1, max1 - min2) : 0.0f;
    result.axis = result.colliding ? axis : vec3(0.0f);
    return result;
}

void main() {
    uint gid = gl_GlobalInvocationID.x;
    if (gid >= collisionCount) return;

    const uint numVertices = objectVertices.length();
    const uint numNormals = objectNormals.length();
    const uint numEdges = objectEdges.length();

    NarrowObject o1;
    o1.idx = collisionPairs[gid].x;
    o1.transform = transforms[o1.idx];

    NarrowObject o2;
    o2.idx = collisionPairs[gid].y;
    o2.transform = transforms[o2.idx];

    Collision collision;
    collision.colliding = true;
    collision.depth = 1e10;
    collision.axis = vec3(0.0f);

    // Test against normals of the first object
    for (uint i = 0; i < numNormals && collision.colliding; i++) {
        vec4 normalVec = o1.transform * objectNormals[i];
        vec3 axis1 = normalize(normalVec.xyz);  // Fixed: Explicitly extract xyz component
        Collision attempt = checkOverlap(axis1, o1.transform, o2.transform, numVertices);
        
        if(!attempt.colliding)
            collision = attempt;
        else
            collision = (attempt.colliding && attempt.depth < collision.depth) ? attempt : collision;
    }

    // Test against normals of the second object
    for (uint i = 0; i < numNormals && collision.colliding; i++) {
        vec4 normalVec = o2.transform * objectNormals[i];
        vec3 axis2 = normalize(normalVec.xyz);  // Fixed: Explicitly extract xyz component
        Collision attempt = checkOverlap(axis2, o1.transform, o2.transform, numVertices);
        
        if(!attempt.colliding)
            collision = attempt;
        else
            collision = (attempt.colliding && attempt.depth < collision.depth) ? attempt : collision;
    }

    // Test cross-products of edges
    for (uint i = 0; i < numEdges && collision.colliding; i++) {
        vec4 edge1 = o1.transform * objectEdges[i];

        for (uint j = 0; j < numEdges && collision.colliding; j++) {
            vec4 edge2 = o2.transform * objectEdges[j];
            vec3 axis = cross(edge1.xyz, edge2.xyz);

            if (length(axis) < 0.001) continue;
            
            axis = normalize(axis);

            Collision attempt = checkOverlap(axis, o1.transform, o2.transform, numVertices);
        
            if(!attempt.colliding)
                collision = attempt;
            else
                collision = (attempt.colliding && attempt.depth < collision.depth) ? attempt : collision;
        }
    }
    
    // If collision is detected, compute the impulse response with respect to the collision normal
    // if (collision.colliding) {
    //     // Get positions from transforms (assuming column 3 contains translation)
    //     vec3 pos1 = o1.transform[3].xyz;
    //     vec3 pos2 = o2.transform[3].xyz;
    //     // Use the normalized vector from o1 to o2 as an approximation of the collision normal
    //     vec3 collisionNormal = collision.axis;

    //     // Retrieve velocities and inverse masses from the properties buffer
    //     vec3 v1 = properties[o1.idx].velocity.xyz;
    //     vec3 v2 = properties[o2.idx].velocity.xyz;
    //     float invMass1 = properties[o1.idx].inverseMass;
    //     float invMass2 = properties[o2.idx].inverseMass;

    //     // Calculate relative velocity along the collision normal
    //     vec3 relVel = v2 - v1;
    //     float relVelAlongNormal = dot(relVel, collisionNormal);

    //     // Only apply impulse if objects are moving towards each other
    //     if (relVelAlongNormal < 0.0) {
    //         float restitution = 1.0; // Coefficient of restitution (bounciness)

    //         // Calculate impulse scalar using the formula:
    //         // j = -(1 + restitution) * (v_rel · n) / (invMass1 + invMass2)
    //         float j = -(1.0 + restitution) * relVelAlongNormal / (invMass1 + invMass2);
    //         vec3 impulse = j * collisionNormal;

    //         // Update velocities with the impulse (Newton's third law)
    //         v1 -= impulse * invMass1;
    //         v2 += impulse * invMass2;
    //         properties[o1.idx].velocity = vec4(v1, 0.0);
    //         properties[o2.idx].velocity = vec4(v2, 0.0);
    //     }
    // }
    if (collision.colliding) {
        // Get positions from transforms (assuming column 3 contains translation)
        vec3 pos1 = o1.transform[3].xyz;
        vec3 pos2 = o2.transform[3].xyz;

        // Define the collision normal from the SAT tests
        vec3 collisionNormal = collision.axis;

        // Approximate the contact point as the midpoint between the two object positions.
        vec3 contactPoint = (pos1 + pos2) * 0.5;

        // Compute the lever arms (offsets from the centers to the contact point)
        vec3 r1 = contactPoint - pos1;
        vec3 r2 = contactPoint - pos2;

        // Retrieve velocities and angular velocities from the properties buffer
        vec3 v1 = properties[o1.idx].velocity.xyz;
        vec3 v2 = properties[o2.idx].velocity.xyz;
        vec3 w1 = properties[o1.idx].angular_velocity.xyz;
        vec3 w2 = properties[o2.idx].angular_velocity.xyz;

        // Compute the velocities at the contact point including rotational contribution:
        // v(contact) = v + cross(angular_velocity, lever_arm)
        vec3 velAtContact1 = v1 + cross(w1, r1);
        vec3 velAtContact2 = v2 + cross(w2, r2);
        vec3 relVel = velAtContact2 - velAtContact1;
        float relVelAlongNormal = dot(relVel, collisionNormal);

        // Only apply impulse if objects are moving toward each other (negative relative velocity)
        if (relVelAlongNormal < 0.0) {
            float restitution = 1.0; // Coefficient of restitution (bounciness)

            // Retrieve inverse masses and inverse inertia tensors
            float invMass1 = properties[o1.idx].inverseMass;
            float invMass2 = properties[o2.idx].inverseMass;
            mat3 invInertia1 = properties[o1.idx].inverseInertiaTensor;
            mat3 invInertia2 = properties[o2.idx].inverseInertiaTensor;

            // Calculate the angular parts
            // Compute the cross products of the lever arms with the collision normal
            vec3 r1CrossN = cross(r1, collisionNormal);
            vec3 r2CrossN = cross(r2, collisionNormal);
            // Compute the angular terms. These represent how much the rotation resists the impulse.
            float angTerm1 = dot(r1CrossN, invInertia1 * r1CrossN);
            float angTerm2 = dot(r2CrossN, invInertia2 * r2CrossN);

            // Compute the impulse scalar
            float j = -(1.0 + restitution) * relVelAlongNormal /
                    (invMass1 + invMass2 + angTerm1 + angTerm2);
            vec3 impulse = j * collisionNormal;

            // Update linear velocities (Newton's Third Law)
            v1 -= impulse * invMass1;
            v2 += impulse * invMass2;
            properties[o1.idx].velocity = vec4(v1, 0.0);
            properties[o2.idx].velocity = vec4(v2, 0.0);

            // Update angular velocities
            // Angular velocity changes based on the torque produced by the impulse, which is given by:
            // deltaAngularVelocity = I^{-1} * (leverArm x impulse)
            w1 -= invInertia1 * cross(r1, impulse);
            w2 += invInertia2 * cross(r2, impulse);
            properties[o1.idx].angular_velocity = vec4(w1, 0.0);
            properties[o2.idx].angular_velocity = vec4(w2, 0.0);
        }
    }
    // Mark both objects in the collision pair (for visualization or debugging)
    secondResults[o1.idx] = collision.colliding ? -1 : 1;
    results[o2.idx] = collision.colliding ? -1 : 1;
}