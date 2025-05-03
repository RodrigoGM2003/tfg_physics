#ifndef PHYSICS_H
#define PHYSICS_H

#pragma once

#include <vector>

#include "glm/glm.hpp"

#include "vertex_buffer.h"

namespace physics{

    /**
     * @brief Axis Aligned Bounding Box
     */
    struct AABB {
        alignas(16) glm::vec3 min;
        alignas(16) glm::vec3 max;
        alignas(16) glm::vec3 center;
        alignas(16) glm::vec3 extents; 
    };

    /**
     * @brief Physics properties of an object
     */
    struct Properties{
        alignas(16) glm::vec3 velocity;
        alignas(16) glm::vec3 acceleration;

        alignas(16) glm::vec3 angular_velocity;
        alignas(16) glm::vec3 angular_acceleration;

        alignas(16) float inverseMass;
        alignas(16) glm::mat3 inverseInertiaTensor;
        
        alignas(16) double friction;
    };

    /**
     * @brief Physics properties of an object
     */
    struct ContactManifold{
        // alignas(16) unsigned int indexA;
        // alignas(16) unsigned int indexB;
        // alignas(16) glm::vec4 normal;          // World space, consistent direction (e.g., A->B)
        // alignas(16) float depth;          // Penetration depth


        unsigned int indexA;
        unsigned int indexB;
        float padding[2];
        glm::vec4 normal;          // World space, consistent direction (e.g., A->B)
        float depth;          // Penetration depth
        float pad[3];                // pad to 48 bytes (if needed for alignment)

        // alignas(16) glm::vec4 pointWorld;      // Contact point in world space
        // alignas(16) unsigned int type;         // Optional: Store collision type (FaceA, FaceB, EdgeEdge)
        // alignas(16) unsigned int featureA;     // Optional: Store feature index on A
        // alignas(16) unsigned int featureB;     // Optional: Store feature index on B

        ContactManifold(){
            indexA = 0;
            indexB = 0;
            normal = glm::vec4(0.0f); 
            depth = 0.0f;
        };
    };

    /**
     * @brief Representation of an object
     */
    struct Object{
        glm::mat4* transform; /* Transformation matrix of the object */
        const std::vector<Vertex>* static_vertices; /* Non-transformed vertices */
        
        AABB aabb; /* Static axis aligned bounding box (for broad phase collision detection) */

        Properties physics; /* Physical properties of an object*/
    };

    /**
     * @brief Representation of an object using gpu
     */
    struct GpuObject{
        glm::mat4* transform; /* Transformation matrix of the object */
        const std::vector<Vertex>* static_vertices; /* Non-transformed vertices */
        
        AABB* aabb; /* Static axis aligned bounding box (for broad phase collision detection) */

        Properties* physics; /* Physical properties of an object*/
    };
}


#endif //PHYSICS_H