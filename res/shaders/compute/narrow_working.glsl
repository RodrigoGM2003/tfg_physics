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

    barrier();
    if(gid == 0){
        collisionCount = 0;
    }
    barrier();

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
    uint featureIndexA = 0; // Index of normal/edge on object A
    uint featureIndexB = 0; // Index of normal/edge on object B (for edge-edge)

    // Test against normals of the first object
    for (uint i = 0; i < numNormals && collision.colliding; i++) {
        vec4 normalVec = o1.transform * objectNormals[i];
        vec3 axis1 = normalize(normalVec.xyz);  // Fixed: Explicitly extract xyz component
        Collision attempt = checkOverlap(axis1, o1.transform, o2.transform, numVertices);
        
        if(!attempt.colliding){
            collision = attempt;
        }
        else{
            collision = attempt.depth < collision.depth ? attempt : collision;
            featureIndexA = i;
        }
    }

    // Test against normals of the second object
    for (uint i = 0; i < numNormals && collision.colliding; i++) {
        vec4 normalVec = o2.transform * objectNormals[i];
        vec3 axis2 = normalize(normalVec.xyz);  // Fixed: Explicitly extract xyz component
        Collision attempt = checkOverlap(axis2, o1.transform, o2.transform, numVertices);
        
        if(!attempt.colliding){
            collision = attempt;
        }
        else{
            collision = attempt.depth < collision.depth ? attempt : collision;
            featureIndexB = i;
        }
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
        
            if(!attempt.colliding){
                collision = attempt;
            }
            else{
                collision = attempt.depth < collision.depth ? attempt : collision;
                featureIndexA = i;
                featureIndexB = j;
            }
        }
    }

    
    vec3 pos1 = o1.transform[3].xyz;
    vec3 pos2 = o2.transform[3].xyz;
    collision.axis = dot(collision.axis, pos2 - pos1) < 0.0 ? -collision.axis : collision.axis;


    if (collision.colliding) {
        ContactManifold contact;
        contact.indexA = o1.idx;
        contact.indexB = o2.idx;
        contact.normal.xyz = collision.axis.xyz;
        contact.depth = collision.depth;


        uint index = atomicAdd(collisionCount, 1);
        manifolds[index] = contact;
    }

    // Mark both objects in the collision pair (for visualization or debugging)
    secondResults[o1.idx] = collision.colliding ? -1 : 1;
    results[o2.idx] = collision.colliding ? -1 : 1;
}