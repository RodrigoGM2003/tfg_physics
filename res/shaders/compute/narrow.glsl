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

bool checkOverlap(vec4 axis, mat4 transform1, mat4 transform2, uint numVertices) {
    // Project first object
    float min1 = dot(axis.xyz, (transform1 * objectVertices[0]).xyz);
    float max1 = min1;
    float min2 = dot(axis.xyz, (transform2 * objectVertices[0]).xyz);
    float max2 = min2;
    
    for (uint i = 1; i < numVertices; i++) {
        float proj1 = dot(axis.xyz, (transform1 * objectVertices[i]).xyz);
        min1 = min(min1, proj1);
        max1 = max(max1, proj1);
        float proj2 = dot(axis.xyz, (transform2 * objectVertices[i]).xyz);
        min2 = min(min2, proj2);
        max2 = max(max2, proj2);
    }
    
    // Check for overlap
    return (min1 <= max2 && max1 >= min2);
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

    bool collision = true;

    // Test against normals of the first object
    for (uint i = 0; i < numNormals && collision; i++) {
        vec4 axis1 = o1.transform * objectNormals[i];
        axis1.xyz = normalize(axis1.xyz);
        collision = checkOverlap(axis1, o1.transform, o2.transform, numVertices);
    }

    // Test against normals of the second object
    for (uint i = 0; i < numNormals && collision; i++) {
        vec4 axis2 = o2.transform * objectNormals[i];
        axis2.xyz = normalize(axis2.xyz);
        collision = checkOverlap(axis2, o1.transform, o2.transform, numVertices);
    }

    // Test cross-products of edges
    for (uint i = 0; i < numEdges && collision; i++) {
        vec4 edge1 = o1.transform * objectEdges[i];
        for (uint j = 0; j < numEdges && collision; j++) {
            vec4 edge2 = o2.transform * objectEdges[j];
            vec3 axis = cross(edge1.xyz, edge2.xyz);
            if (length(axis) < 0.001) continue;
            axis = normalize(axis);
            collision = checkOverlap(vec4(axis, 0.0), o1.transform, o2.transform, numVertices);
        }
    }
    
    // If collision is detected, compute the impulse response with respect to the collision normal
    if (collision) {
        // Get positions from transforms (assuming column 3 contains translation)
        vec3 pos1 = o1.transform[3].xyz;
        vec3 pos2 = o2.transform[3].xyz;
        // Use the normalized vector from o1 to o2 as an approximation of the collision normal
        vec3 collisionNormal = normalize(pos2 - pos1);

        // Retrieve velocities and inverse masses from the properties buffer
        vec3 v1 = properties[o1.idx].velocity.xyz;
        vec3 v2 = properties[o2.idx].velocity.xyz;
        float invMass1 = properties[o1.idx].inverseMass;
        float invMass2 = properties[o2.idx].inverseMass;

        // Calculate relative velocity along the collision normal
        vec3 relVel = v2 - v1;
        float relVelAlongNormal = dot(relVel, collisionNormal);

        // Only apply impulse if objects are moving towards each other
        if (relVelAlongNormal < 0.0) {
            float restitution = 1.0; // Coefficient of restitution (bounciness)

            // Calculate impulse scalar using the formula:
            // j = -(1 + restitution) * (v_rel · n) / (invMass1 + invMass2)
            float j = -(1.0 + restitution) * relVelAlongNormal / (invMass1 + invMass2);
            vec3 impulse = j * collisionNormal;

            // Update velocities with the impulse (Newton's third law)
            v1 -= impulse * invMass1;
            v2 += impulse * invMass2;
            properties[o1.idx].velocity = vec4(v1, 0.0);
            properties[o2.idx].velocity = vec4(v2, 0.0);
        }
    }

    // Mark both objects in the collision pair (for visualization or debugging)
    secondResults[o1.idx] = collision ? -1 : 1;
    results[o2.idx] = collision ? -1 : 1;
}


// #version 430 core

// #include "common_structs.glsl"

// layout(std430, binding = 1) buffer TransformBuffer {
//     mat4 transforms[];
// };

// layout(std430, binding = 4) buffer ResultsBuffer {
//     int results[];
// };

// layout(std430, binding = 20) buffer CollisionPairsBuffer {
//     ivec2 collisionPairs[];
// };

// layout(std430, binding = 21) buffer CollisionCountBuffer{
//     uint collisionCount;
// };

// layout(std430, binding = 22) buffer SecondResultsBuffer {
//     int secondResults[];
// };

// layout(std430, binding = 23) buffer ObjectVerticesBuffer {
//     vec4 objectVertices[];
// };

// layout(std430, binding = 24) buffer ObjectNormalsBuffer {
//     vec4 objectNormals[];
// };

// layout(std430, binding = 25) buffer ObjectEdgesBuffer {
//     vec4 objectEdges[];
// };

// layout(std430, binding = 5) buffer PropertiesBuffer {
//     PropertiesStruct properties[];
// };

// layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

// struct NarrowObject{
//     uint idx;
//     mat4 transform;
// };

// bool checkOverlap(vec4 axis, mat4 transform1, mat4 transform2, uint numVertices) {
//     // Project first object
//     float min1 = dot(axis.xyz, (transform1 * objectVertices[0]).xyz);
//     float max1 = min1;
//     float min2 = dot(axis.xyz, (transform2 * objectVertices[0]).xyz);
//     float max2 = min2;
    
//     for (uint i = 1; i < numVertices; i++) {
//         float proj1 = dot(axis.xyz, (transform1 * objectVertices[i]).xyz);
//         min1 = min(min1, proj1);
//         max1 = max(max1, proj1);
//         float proj2 = dot(axis.xyz, (transform2 * objectVertices[i]).xyz);
//         min2 = min(min2, proj2);
//         max2 = max(max2, proj2);
//     }
    
//     // Check for overlap
//     return (min1 <= max2 && max1 >= min2);
// }

// void main() {
//     uint gid = gl_GlobalInvocationID.x;

//     if (gid >= collisionCount) return;

//     const uint numVertices = objectVertices.length();
//     const uint numNormals = objectNormals.length();
//     const uint numEdges = objectEdges.length();

//     NarrowObject o1;
//     o1.idx = collisionPairs[gid].x;
//     o1.transform = transforms[o1.idx];

//     NarrowObject o2;
//     o2.idx = collisionPairs[gid].y;
//     o2.transform = transforms[o2.idx];

//     bool collision = true;

//     for(uint i = 0; i < numNormals && collision; i++){
//         vec4 axis1 = o1.transform * objectNormals[i];
//         axis1.xyz = normalize(axis1.xyz);
//         collision = checkOverlap(axis1, o1.transform, o2.transform, numVertices);
//     }

//     for(uint i = 0; i < numNormals && collision; i++){
//         vec4 axis2 = o2.transform * objectNormals[i];
//         axis2.xyz = normalize(axis2.xyz);
//         collision = checkOverlap(axis2, o1.transform, o2.transform, numVertices);
//     }

//     for(uint i = 0; i < numEdges && collision; i++){
//         vec4 edge1 = o1.transform * objectEdges[i];

//         for(uint j = 0; j < numEdges && collision; j++){
//             vec4 edge2 = o2.transform * objectEdges[j];

//             vec3 axis = cross(edge1.xyz, edge2.xyz);

//             if(length(axis) < 0.001) continue;

//             axis = normalize(axis);

//             collision = checkOverlap(vec4(axis, 0.0), o1.transform, o2.transform, numVertices);
//         }
//     }


//     // Mark both objects in the collision pair
//     secondResults[o1.idx] = collision ? -1 : 1;      // First object as green
//     results[o2.idx] = collision ? -1 : 1;      // First object as red
// }