#version 450 core

#include "common_structs.glsl" // Assuming ContactManifold is also in here or defined above

// --- Buffer Definitions ---
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
    uint collisionCount; // Input: Max pairs, Output: Actual collision count
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
    uint objectEdgesIndices[];
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
    vec4 pointWorld;      // Contact point in world space
    // uint type;         // Optional: Store collision type (FaceA, FaceB, EdgeEdge)
    // uint featureA;     // Optional: Store feature index on A
    // uint featureB;     // Optional: Store feature index on B
};

// Add the new output buffer for manifolds
layout(std430, binding = 26) buffer ContactManifoldBuffer {
    ContactManifold manifolds[];
};
// --- End Buffer Definitions ---

layout(local_size_x = 256, local_size_y = 1, local_size_z = 1) in;

// --- Struct Definitions ---
struct NarrowObject {
    uint idx;
    mat4 transform;
};

const uint AXIS_TYPE_FACE_A = 1;
const uint AXIS_TYPE_FACE_B = 2;
const uint AXIS_TYPE_EDGE_EDGE = 3;

// ContactManifold struct should be defined here or included
// struct ContactManifold { ... };

const uint MAX_VERTICES = 32;
vec4 worldVertices[2][MAX_VERTICES]; // Use shared memory if MAX_VERTICES is large and reused often? Consider lifetime.
const uint MAX_EDGES = 64;
vec4 worldEdges[2][MAX_EDGES]; // Same consideration as worldVertices

struct Collision {
    bool colliding;
    vec3 axis;         // Final oriented axis
    float depth;
    // vec3 worldPoint; // This member seems redundant now, use contactPoint vec4
    uint type;         // Type of collision that determined minimum depth
};
// --- End Struct Definitions ---


// --- Helper Functions (transformVertices, getWorldEdges, checkOverlap) ---
// (Keep these functions as they are)
void transformVertices(NarrowObject o1, NarrowObject o2, uint nVertices){
    for(uint i = 0; i < nVertices; i++){
        worldVertices[0][i] = o1.transform * objectVertices[i];
        worldVertices[1][i] = o2.transform * objectVertices[i];
    }
}

void getWorldEdges(NarrowObject o1, NarrowObject o2, uint nEdges){
    for(uint i = 0; i < nEdges; i++){
        worldEdges[0][i] = vec4(worldVertices[0][objectEdgesIndices[(i * 2) + 1]].xyz - worldVertices[0][objectEdgesIndices[i * 2]].xyz, 0);
        worldEdges[1][i] = vec4(worldVertices[1][objectEdgesIndices[(i * 2) + 1]].xyz - worldVertices[1][objectEdgesIndices[i * 2]].xyz, 0);
    }
}

Collision checkOverlap(const vec3 axis, const mat4 transform1, const mat4 transform2, const uint numVertices) {
    float min1 = dot(axis, worldVertices[0][0].xyz);
    float max1 = min1;
    float min2 = dot(axis, worldVertices[1][0].xyz);
    float max2 = min2;

    for (uint i = 1; i < numVertices; i++) {
        float proj1 = dot(axis, worldVertices[0][i].xyz);
        min1 = min(min1, proj1);
        max1 = max(max1, proj1);
        float proj2 = dot(axis, worldVertices[1][i].xyz);
        min2 = min(min2, proj2);
        max2 = max(max2, proj2);
    }

    Collision result;
    result.colliding = (min1 <= max2 && max1 >= min2);
    result.depth = result.colliding ? min(max2 - min1, max1 - min2) : 0.0f;
    // Store the axis from the check temporarily, it might be flipped later
    result.axis = result.colliding ? axis : vec3(0.0f);
    result.type = 0; // Type is set later based on which check passes
    return result;
}
// --- End Helper Functions ---


void main() {
    uint gid = gl_GlobalInvocationID.x;

    // Get the total number of pairs to check (passed from CPU)
    // Note: This assumes collisionCount buffer initially holds the max count.
    uint initialPairCount = collisionCount; // Read before resetting

    // Reset the actual collision counter using the first thread
    // Ensure all threads see the initial count before reset happens, and see the reset after.
    barrier();
    if(gid == 0){
        collisionCount = 0; // Reset actual collision count
    }
    barrier(); // Ensure all threads see the reset count (0)

    // Check if this thread is processing a valid pair index
    if (gid >= initialPairCount) return;


    const uint numVertices = objectVertices.length();
    const uint numNormals = objectNormals.length();
    const uint numEdges = objectEdgesIndices.length() / 2;

    // --- Get Objects and Transform Data ---
    NarrowObject o1;
    o1.idx = collisionPairs[gid].x;
    o1.transform = transforms[o1.idx];

    NarrowObject o2;
    o2.idx = collisionPairs[gid].y;
    o2.transform = transforms[o2.idx];

    transformVertices(o1, o2, numVertices);
    getWorldEdges(o1, o2, numEdges);
    // --- End Data Transform ---

    // --- SAT Checks ---
    Collision collision; // Holds the minimum separation axis info if collision occurs
    collision.colliding = true; // Assume colliding until proven otherwise
    collision.depth = 1e10;
    collision.axis = vec3(0.0f); // Will store the axis of minimum penetration
    collision.type = 0;         // Will store the type (FaceA/B, EdgeEdge)

    uint featureIndexA = 0; // Index of normal/edge on object A (o1)
    uint featureIndexB = 0; // Index of normal/edge on object B (o2) (only used for edge-edge)

    // Temp variable to hold results from checkOverlap
    Collision attempt;

    // Test against normals of the first object (o1)
    for (uint i = 0; i < numNormals && collision.colliding; i++) {
        vec4 normalVec = o1.transform * objectNormals[i]; // World space normal
        vec3 axis = normalize(normalVec.xyz);
        attempt = checkOverlap(axis, o1.transform, o2.transform, numVertices);

        if (!attempt.colliding) {
            collision.colliding = false; // Found separating axis
            break; // Exit loop early
        } else {
            if (attempt.depth < collision.depth) {
                collision.depth = attempt.depth;
                collision.axis = attempt.axis; // Store the axis from checkOverlap
                collision.type = AXIS_TYPE_FACE_A;
                featureIndexA = i; // Store index of the normal on o1
            }
        }
    }

    // Test against normals of the second object (o2)
    if (collision.colliding) { // Only continue if still potentially colliding
        for (uint i = 0; i < numNormals && collision.colliding; i++) {
            vec4 normalVec = o2.transform * objectNormals[i]; // World space normal
            vec3 axis = normalize(normalVec.xyz);
            attempt = checkOverlap(axis, o1.transform, o2.transform, numVertices);

            if (!attempt.colliding) {
                collision.colliding = false;
                break;
            } else {
                if (attempt.depth < collision.depth) {
                    collision.depth = attempt.depth;
                    collision.axis = attempt.axis;
                    collision.type = AXIS_TYPE_FACE_B;
                    featureIndexB = i; // Store index of the normal on o2
                }
            }
        }
    }

    // // Test cross-products of edges
    if (collision.colliding) { // Only continue if still potentially colliding
        for (uint i = 0; i < numEdges && collision.colliding; i++) {
            vec4 edge1 = worldEdges[0][i]; // Edge from o1

            for (uint j = 0; j < numEdges && collision.colliding; j++) {
                vec4 edge2 = worldEdges[1][j]; // Edge from o2
                vec3 axisCross = cross(edge1.xyz, edge2.xyz);

                if (length(axisCross) < 0.001) continue; // Skip parallel edges

                vec3 axis = normalize(axisCross);
                attempt = checkOverlap(axis, o1.transform, o2.transform, numVertices);

                if (!attempt.colliding) {
                    collision.colliding = false;
                    break; // Break inner loop
                } else if (attempt.depth < collision.depth) {
                    collision.depth = attempt.depth;
                    collision.axis = attempt.axis;
                    collision.type = AXIS_TYPE_EDGE_EDGE;
                    featureIndexA = i; // Store index of edge on o1
                    featureIndexB = j; // Store index of edge on o2
                }
                
            }
             if (!collision.colliding) break; // Break outer loop if separated
        }
    }
    // --- End SAT Checks ---


    // --- Contact Point Calculation and Manifold Creation ---
    if (collision.colliding) {
        uint manifoldIndex = atomicAdd(collisionCount, 1);


        vec4 contactPointWorld = vec4(0.0); // Final contact point

        vec3 centerToCenter = o2.transform[3].xyz - o1.transform[3].xyz;
        vec3 finalManifoldNormal = collision.axis; // Start with the axis from SAT
        if (dot(centerToCenter, finalManifoldNormal) < 0.0) {
            finalManifoldNormal = -finalManifoldNormal; // Flip if pointing B->A
        }

        if (collision.type == AXIS_TYPE_FACE_A || collision.type == AXIS_TYPE_FACE_B) {
            bool typeCheck = collision.type == AXIS_TYPE_FACE_A; // True if o1's face normal was axis

            NarrowObject incident = typeCheck ? o2 : o1;
            incident.idx = typeCheck ? 1 : 0; // Index into worldVertices/worldEdges

            NarrowObject reference = typeCheck ? o1 : o2;
            reference.idx = typeCheck ? 0 : 1; // Index into worldVertices/worldEdges         

            // vec3 centerToCenter = incident.transform[3].xyz - reference.transform[3].xyz;
            // if (dot(centerToCenter, collision.axis) < 0.0) {
            //     collision.axis = -collision.axis;
            // }

            float minProjection = 1e10;
            vec4 deepestVertex = vec4(0.0); // Store the vertex itself

            for (uint i = 0; i < numVertices; i++) {
                vec4 vertex = worldVertices[incident.idx][i]; // Vertex from incident object
                float proj = dot(vertex.xyz - reference.transform[3].xyz, collision.axis);

                if (proj < minProjection) {
                    minProjection = proj;
                    deepestVertex = vertex;
                }
            }
            contactPointWorld = deepestVertex; // Simplest contact point: the vertex itself

        }
        else if (collision.type == AXIS_TYPE_EDGE_EDGE) {
            vec3 edgeADir = worldEdges[0][featureIndexA].xyz; // Edge from o1
            vec3 edgeBDir = worldEdges[1][featureIndexB].xyz; // Edge from o2

            uint vA1_idx = objectEdgesIndices[featureIndexA * 2];
            uint vB1_idx = objectEdgesIndices[featureIndexB * 2];

            vec3 pA1_world = worldVertices[0][vA1_idx].xyz; // Start point of edge A (o1)
            vec3 pB1_world = worldVertices[1][vB1_idx].xyz; // Start point of edge B (o2)

            vec3 r = pB1_world - pA1_world;
            float a = dot(edgeADir, edgeADir);
            float b = dot(edgeADir, edgeBDir);
            float c = dot(edgeBDir, edgeBDir);
            float d = dot(edgeADir, r);
            float e = dot(edgeBDir, r);
            float denom = a*c - b*b;
            float t1, t2;

            if (a < 0.0001 || c < 0.0001 || abs(denom) < 0.0001) {
                t1 = 0.0; t2 = 0.0; // Degenerate case fallback
            } else {
                t1 = clamp((b*e - c*d) / denom, 0.0, 1.0);
                t2 = clamp((a*e - b*d) / denom, 0.0, 1.0);
            }

            vec3 closestA = pA1_world + edgeADir * t1;
            vec3 closestB = pB1_world + edgeBDir * t2;
            contactPointWorld = vec4(((closestA + closestB) * 0.5), 1.0);

            // vec3 centerToCenter = o2.transform[3].xyz - o1.transform[3].xyz;
            //  if (dot(centerToCenter, collision.axis) < 0.0) {
            //     collision.axis = -collision.axis;
            // }
        }

        ContactManifold manifold;
        manifold.indexA = o1.idx;
        manifold.indexB = o2.idx;
        vec3 aToB = o2.transform[3].xyz - o1.transform[3].xyz;
        manifold.normal = vec4(finalManifoldNormal, 0.0); // Use the final oriented normal
        // manifold.normal = dot(aToB, collision.axis) < 0.0 ? vec4(-collision.axis, 0.0) : vec4(collision.axis, 0.0); // Use the finalized, oriented axis
        manifold.depth = collision.depth;
        manifold.pointWorld = contactPointWorld;

        manifolds[manifoldIndex] = manifold;
    } 
    // --- End Contact Point Calculation and Manifold Creation ---

    // Mark both objects in the collision pair (for visualization or debugging)
    secondResults[o1.idx] = collision.colliding ? -1 : 1;
    results[o2.idx] = collision.colliding ? -1 : 1;
}