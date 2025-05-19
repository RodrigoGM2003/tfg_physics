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

    vec4 rALocal;
    vec4 rBLocal;
};

// Add the new output buffer for manifolds
layout(std430, binding = 26) buffer ContactManifoldBuffer {
    ContactManifold manifolds[];
};


layout(local_size_x = 512, local_size_y = 1, local_size_z = 1) in;

struct NarrowObject {
    uint idx;
    mat4 transform;
};


struct Collision {
    bool colliding;
    vec3 axis;
    float depth;
    bool aOnB;
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

vec3 getSupportVertexWorld(mat4 transform, vec3 direction, uint numObjectVertices) {
    float maxProjection = -1e30f;
    float epsilon = 1e-3f;

    // First pass: find the max projection
    for (uint i = 0; i < numObjectVertices; ++i) {
        vec3 vWorld = (transform * objectVertices[i]).xyz;
        float projection = dot(vWorld, direction);
        if (projection > maxProjection) {
            maxProjection = projection;
        }
    }

    // Second pass: average all vertices within epsilon of max
    vec3 supportSum = vec3(0.0f);
    uint count = 0u;
    for (uint i = 0; i < numObjectVertices; ++i) {
        vec3 vWorld = (transform * objectVertices[i]).xyz;
        float projection = dot(vWorld, direction);
        if (abs(projection - maxProjection) < epsilon) {
            supportSum += vWorld;
            count += 1u;
        }
    }

    return count > 0u ? supportSum / float(count) : vec3(0.0f);
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

    bool aOnB = true;

    // Test against normals of the first object
    for (uint i = 0; i < numNormals && collision.colliding; i++) {
        vec4 normalVec = o1.transform * objectNormals[i];
        vec3 axis1 = normalize(normalVec.xyz);  // Fixed: Explicitly extract xyz component
        Collision attempt = checkOverlap(axis1, o1.transform, o2.transform, numVertices);
        attempt.aOnB = true;
        
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
        attempt.aOnB = false;
        
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
    vec3 cTC = pos2 - pos1;


    if (collision.colliding) {
        ContactManifold contact;
        contact.indexA = o1.idx;
        contact.indexB = o2.idx;
        contact.normal.xyz = collision.axis; // Already normalized and pointing A->B
        contact.depth = collision.depth;

        // Approximate contact point using support vertices
        // This is a common way to get a reasonable contact point from SAT.
        // vec3 supportA_world = getSupportVertexWorld(o1.transform, collision.axis, numVertices);
        // vec3 supportB_world = getSupportVertexWorld(o2.transform, -collision.axis, numVertices);
        vec3 supportA_world = getSupportVertexWorld(o1.transform, collision.axis, numVertices);
        vec3 supportB_world = getSupportVertexWorld(o2.transform, -collision.axis, numVertices);


        // The contact point can be on A, on B, or halfway.
        // Point on A's surface: supportA_world
        // Point on B's surface: supportB_world
        // Midpoint:

        // Alternative: Project one support point onto the other object's plane along the normal.
        // E.g., take supportA and project it towards B along the normal by depth.
        vec3 contactPointWorld = supportA_world - collision.axis * collision.depth; // If supportA is deepest point on A

        // Using the midpoint of supports is a decent general approximation:
        // contact.rALocal.xyz = contactPointWorld - pos1;
        // contact.rBLocal.xyz = contactPointWorld - pos2;
        // contact.rALocal.xyz = collision.aOnB ? supportB_world : supportA_world ;
        // contact.rALocal.xyz -= pos1;
        // contact.rBLocal.xyz = collision.aOnB ? supportB_world : supportA_world ;
        // contact.rBLocal.xyz -= pos2;
        contact.rALocal.xyz = supportA_world - pos1;
        contact.rBLocal.xyz = supportB_world - pos2;

        // If you have 'radius' in PropertiesStruct and want sphere-like contacts:
        // (This would require PropertiesBuffer here)
        // float radiusA = properties[o1.idx].radius;
        // float radiusB = properties[o2.idx].radius;
        // vec3 P_on_A = centerA + collision.axis * radiusA;
        // vec3 P_on_B = centerB - collision.axis * radiusB;
        // vec3 contactPointWorld_sphere = P_on_A - collision.axis * ( (radiusA + radiusB - length(centerA-centerB) + collision.depth ) * radiusA / (radiusA+radiusB) ); // Complex, just an idea
        // A simpler sphere contact:
        // contactPointWorld_sphere = centerA + collision.axis * (radiusA - collision.depth * 0.5f);
        // contact.rA_world = contactPointWorld_sphere - centerA;
        // contact.rB_world = contactPointWorld_sphere - centerB;


        // Output the manifold
        uint output_manifold_idx = atomicAdd(collisionCount, 1); // collisionCount here is the output counter
        if(output_manifold_idx < manifolds.length()) { // manifolds.length() is max capacity
            manifolds[output_manifold_idx] = contact;
        }
    }

    // Mark both objects in the collision pair (for visualization or debugging)
    // secondResults[o1.idx] = collision.colliding ? -1 : 1;
    // results[o2.idx] = collision.colliding ? -1 : 1;
}